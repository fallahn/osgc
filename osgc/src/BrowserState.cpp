/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "BrowserState.hpp"
#include "States.hpp"
#include "ResourceIDs.hpp"
#include "Game.hpp"
#include "SliderSystem.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "BrowserNodeSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/core/ConfigFile.hpp>
#include <xyginext/core/Console.hpp>
#include <xyginext/gui/Gui.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

#include <fstream>
#include <cstring>

namespace
{
#if defined(__APPLE__)
    const std::string pluginFile("/libosgc.dylib");
    const std::string pluginFolder("plugins/");
    const std::string infoFile("/info.xgi");
#elif defined(_WIN32)
    const std::string pluginFile("\\osgc.dll");
    const std::string pluginFolder("plugins\\");
    const std::string infoFile("\\info.xgi");
#else
    const std::string pluginFile("/libosgc.so");
    const std::string pluginFolder("plugins/");
    const std::string infoFile("/info.xgi");
#endif

    const std::string settingsFile("browser.cfg");

    struct PluginInfo final
    {
        std::string name;
        std::string thumbnail;
        std::string author;
        std::string version;
    };

    template <typename T>
    sf::Vector2<T> operator / (sf::Vector2<T> a, sf::Vector2<T> b)
    {
        return { a.x / b.x, a.y / b.y };
    }
    
    const float ItemPadding = 20.f;

    const int BackgroundDepth = -20;
    const int ArrowDepth = -10;
    const int TextDepth = 1;

    const std::string SlideshowFrag = 
        R"(
            #version 120

            uniform sampler2D u_texA;
            uniform sampler2D u_texB;
            uniform float u_mix;

            void main()
            {
                vec4 imgA = texture2D(u_texA, gl_TexCoord[0].xy);
                vec4 imgB = texture2D(u_texB, gl_TexCoord[0].xy);
    
                gl_FragColor = mix(imgA, imgB, u_mix);
            }
          )";

    struct SlideshowState final
    {
        enum
        {
            In, Hold, Out
        }state = In;
        bool pendingActiveChange = false;
        float easeTime = 0.f;
        float delayTime = 0.f;
        static constexpr float HoldTime = 15.f;
    };
}

BrowserState::BrowserState(xy::StateStack& ss, xy::State::Context ctx, Game& game)
    : xy::State         (ss, ctx),
    m_gameInstance      (game),
    m_scene             (ctx.appInstance.getMessageBus()),
    m_browserTargetIndex(0),
    m_slideshowIndex    (0)
{
    launchLoadingScreen();
    
    //make sure to unload any active plugin so our asset directory is correct
    game.unloadPlugin();
    
    loadSettings();

    initScene();
    loadResources();
    buildMenu();

    registerConsoleTab("Options",
        [&]() 
        {
            auto oldSlideshow = m_settings.useSlideshow;
            xy::Nim::checkbox("Slideshow Background", &m_settings.useSlideshow);
            xy::Nim::sameLine(); xy::Nim::showToolTip("Displays images stored in the /images/backgrounds/ directory as a slideshow instead of the selected game background.");
            if (oldSlideshow != m_settings.useSlideshow)
            {
                xy::Command cmd;
                cmd.targetFlags = CommandID::Slideshow;
                cmd.action = [oldSlideshow](xy::Entity e, float)
                {
                    if (oldSlideshow)
                    {
                        auto& data = std::any_cast<SlideshowState&>(e.getComponent<xy::Callback>().userData);
                        data.pendingActiveChange = true;
                    }
                    else
                    {
                        e.getComponent<xy::Callback>().active = true;
                    }
                };
                m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                saveSettings();
            };
        });

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    ctx.appInstance.setMouseCursorVisible(true);

    quitLoadingScreen();
}

BrowserState::~BrowserState()
{
    saveSettings();
}

bool BrowserState::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::Left:
            prevItem();
            break;
        case sf::Keyboard::Right:
            nextItem();
            break;
        case sf::Keyboard::Space:
        case sf::Keyboard::Enter:
        //case sf::Keyboard::Return:
            execItem();
            break;
        case sf::Keyboard::Escape:
            xy::Console::show();
            break;
        }
    }
    else if (evt.type == sf::Event::JoystickButtonReleased)
    {
        //TODO controller mappings.
        switch (evt.joystickButton.button)
        {
        default: break;
        case 0:
            execItem();
            break;
        case 7:
            xy::Console::show();
            break;
        }
    }
    else if (evt.type == sf::Event::JoystickMoved)
    {
        if (evt.joystickMove.axis == sf::Joystick::PovX)
        {
            //TODO this. When we have a controller.
        }
    }

    m_scene.getSystem<xy::UISystem>().handleEvent(evt);
    m_scene.forwardEvent(evt);
    return true;
}

void BrowserState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::SliderMessage)
    {
        const auto& data = msg.getData<SliderEvent>();
        if (data.action == SliderEvent::Finished)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::BrowserNode;
            cmd.action = [&](xy::Entity e, float)
            {
                auto& node = e.getComponent<BrowserNode>();
                node.enabled = node.index == m_browserTargetIndex;

                if (node.enabled)
                {
                    xy::Command cmd2;
                    cmd2.targetFlags = CommandID::TitleText;
                    cmd2.action = [&node](xy::Entity ee, float)
                    {
                        ee.getComponent<xy::Text>().setString(node.title);
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd2);
                }
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            //TODO update title text
        }
    }

    m_scene.forwardMessage(msg);
}

bool BrowserState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void BrowserState::draw()
{
    auto rw = getContext().appInstance.getRenderWindow();
    rw->draw(m_scene);
}

xy::StateID BrowserState::stateID() const
{
    return States::BrowserState;
}

//private
void BrowserState::loadSettings()
{
    auto path = xy::FileSystem::getConfigDirectory(getContext().appInstance.getApplicationName()) + settingsFile;
    std::ifstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        file.seekg(0, file.end);
        auto fileSize = file.tellg();
        if (fileSize < sizeof(Settings))
        {
            xy::Logger::log(path + " invalid file size.", xy::Logger::Type::Error);
            return;
        }
        file.seekg(file.beg);

        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);
        file.close();

        std::memcpy(&m_settings, buffer.data(), buffer.size());
    }
    else
    {
        file.close();
        xy::Logger::log("failed opening " + path + " for reading - does it not yet exist?", xy::Logger::Type::Error);
    }
}

void BrowserState::saveSettings()
{
    std::vector<char> buffer(sizeof(Settings));
    std::memcpy(buffer.data(), &m_settings, sizeof(Settings));

    auto path = xy::FileSystem::getConfigDirectory(getContext().appInstance.getApplicationName()) + settingsFile;
    std::ofstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        file.write(buffer.data(), buffer.size());
        file.close();
    }
    else
    {
        file.close();
        xy::Logger::log("failed opening " + path + " for writing - does it not yet exist?", xy::Logger::Type::Error);
    }
}

void BrowserState::initScene()
{
    //add the systems
    auto& messageBus = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(messageBus);
    m_scene.addSystem<BrowserNodeSystem>(messageBus);
    m_scene.addSystem<SliderSystem>(messageBus);
    m_scene.addSystem<xy::CallbackSystem>(messageBus);
    m_scene.addSystem<xy::TextSystem>(messageBus);
    m_scene.addSystem<xy::UISystem>(messageBus);
    m_scene.addSystem<xy::SpriteSystem>(messageBus);
    m_scene.addSystem<xy::RenderSystem>(messageBus);
}

void BrowserState::loadResources()
{
    //load resources
    FontID::handles[FontID::MenuFont] = m_resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");
    
    TextureID::handles[TextureID::Fallback] = m_resources.load<sf::Texture>("fallback");
    TextureID::handles[TextureID::Background] = m_resources.load<sf::Texture>("assets/images/background.png");
    TextureID::handles[TextureID::BackgroundShine] = m_resources.load<sf::Texture>("assets/images/shine.png");
    TextureID::handles[TextureID::DefaultThumb] = m_resources.load<sf::Texture>("assets/images/default_thumb.png");
    TextureID::handles[TextureID::Arrow] = m_resources.load<sf::Texture>("assets/images/navigation.png");
    TextureID::handles[TextureID::Options] = m_resources.load<sf::Texture>("assets/images/options.png");
}

void BrowserState::buildMenu()
{
    //parses any info files found
    auto parseInfo = [](const std::string& path, PluginInfo& output)
    {
        xy::ConfigFile cfg;
        if (cfg.loadFromFile(path))
        {
            if (auto prop = cfg.findProperty("title"); prop != nullptr)
            {
                output.name = prop->getValue<std::string>();
            }

            if (auto prop = cfg.findProperty("thumb"); prop != nullptr)
            {
                output.thumbnail = prop->getValue<std::string>();
            }

            if (auto prop = cfg.findProperty("version"); prop != nullptr)
            {
                output.version= prop->getValue<std::string>();
            }

            if (auto prop = cfg.findProperty("author"); prop != nullptr)
            {
                output.author = prop->getValue<std::string>();
            }
        }
    };

    static const auto ThumbnailSize = xy::DefaultSceneSize / 2.f; //make sure to scale all sprites to correct size
    auto applyVertices = [](xy::Drawable & drawable)
    {
        auto& verts = drawable.getVertices();
        verts.resize(8);
        verts[0] = { sf::Vector2f(), sf::Vector2f() };
        verts[1] = { sf::Vector2f(ThumbnailSize.x, 0.f), sf::Vector2f(ThumbnailSize.x, 0.f) };
        verts[2] = { sf::Vector2f(0.f, ThumbnailSize.y), sf::Vector2f(0.f, ThumbnailSize.y) };
        verts[3] = { ThumbnailSize, ThumbnailSize };
        verts[4] = { sf::Vector2f(0.f, ThumbnailSize.y), sf::Color(255,255,255,120), sf::Vector2f(0.f, ThumbnailSize.y) };
        verts[5] = { ThumbnailSize, sf::Color(255,255,255,120), ThumbnailSize };
        verts[6] = { sf::Vector2f(0.f, ThumbnailSize.y * 1.4f), sf::Color::Transparent, sf::Vector2f(0.f, 0.f) };
        verts[7] = { sf::Vector2f(ThumbnailSize.x, ThumbnailSize.y * 1.4f), sf::Color::Transparent, sf::Vector2f(ThumbnailSize.x, 0.f) };

        drawable.updateLocalBounds();
        drawable.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
    };

    //background
    buildSlideshow();

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Background]));

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y * 0.6f);
    entity.addComponent<xy::Drawable>().setDepth(BackgroundDepth + 2);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::BackgroundShine]));
    auto shineWidth = entity.getComponent<xy::Sprite>().getTextureBounds().width;
    auto scale = xy::DefaultSceneSize.x / shineWidth;
    entity.getComponent<xy::Transform>().setScale(scale, 1.f);

    auto & menuFont = m_resources.get<sf::Font>(FontID::handles[FontID::MenuFont]);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ItemPadding, ItemPadding);
    entity.addComponent<xy::Text>(menuFont).setString("OSGC");
    entity.getComponent<xy::Text>().setCharacterSize(80);
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Options]));
    auto optionBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - (optionBounds.width + ItemPadding), ItemPadding);
    entity.addComponent<xy::UIHitBox>().area = optionBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [](xy::Entity, sf::Uint64 flags) 
            {
                xy::Console::show();
            });

    //navigation arrows
    entity = m_scene.createEntity();    
    entity.addComponent<xy::Drawable>().setDepth(ArrowDepth);
    auto arrowBounds = entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Arrow])).getTextureBounds();
    entity.addComponent<xy::Transform>().setOrigin(arrowBounds.width / 2.f, arrowBounds.height / 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(ThumbnailSize.x * 0.6f, 0.f);
    entity.addComponent<xy::UIHitBox>().area = arrowBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    nextItem();
                }
            });

    entity = m_scene.createEntity();
    entity.addComponent<xy::Drawable>().setDepth(ArrowDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Arrow]));
    entity.addComponent<xy::Transform>().setOrigin(arrowBounds.width / 2.f, arrowBounds.height / 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(-ThumbnailSize.x * 0.6f, 0.f);
    entity.getComponent<xy::Transform>().setScale(-1.f, 1.f);
    entity.addComponent<xy::UIHitBox>().area = arrowBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    prevItem();
                }
            });

    //root node - game nodes are connected to this which controls the slide
    entity = m_scene.createEntity();
    auto& rootNode = entity.addComponent<xy::Transform>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::RootNode;
    entity.addComponent<Slider>().speed = 15.f;
    entity.getComponent<Slider>().active = true; //ensures first node is selected.

    //game plugin nodes
    sf::Vector2f nodePosition = xy::DefaultSceneSize / 2.f;
    nodePosition.y += (ThumbnailSize.y * 0.6f) / 2.f;
    const sf::Vector2f basePosition = nodePosition;
    m_browserTargets.emplace_back();

    std::size_t i = 0;

    auto pluginList = xy::FileSystem::listDirectories(xy::FileSystem::getResourcePath() + pluginFolder);
    std::sort(pluginList.begin(), pluginList.end()); //not all OSs list alphabetically
    for (const auto& dir : pluginList)
    {
        //check plugin exists
        if (!xy::FileSystem::fileExists(xy::FileSystem::getResourcePath() + pluginFolder + dir + pluginFile))
        {
            LOG(xy::FileSystem::getResourcePath() + pluginFolder + dir + pluginFile, xy::Logger::Type::Error);
            LOG("plugin file not found", xy::Logger::Type::Error);
            continue;
        }

        //check for info file and parse if exists
        PluginInfo info;
        info.name = dir;

        auto infoPath = xy::FileSystem::getResourcePath() + pluginFolder + dir + infoFile;
        if (xy::FileSystem::fileExists(infoPath))
        {
            parseInfo(infoPath, info);
        }

        auto thumbID = TextureID::handles[TextureID::DefaultThumb];
        if (!info.thumbnail.empty())
        {
            thumbID = m_resources.load<sf::Texture>(pluginFolder + dir + "/" + info.thumbnail);

            if (thumbID == TextureID::handles[TextureID::Fallback])
            {
                thumbID = TextureID::handles[TextureID::DefaultThumb];
            }
        }

        //create entity
        auto loadPath = pluginFolder + dir;

        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(nodePosition);
        entity.getComponent<xy::Transform>().setOrigin(ThumbnailSize.x / 2.f, ThumbnailSize.y * 0.8f);
        entity.addComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(thumbID));
        applyVertices(entity.getComponent<xy::Drawable>());
        entity.addComponent<BrowserNode>().index = i++;
        entity.getComponent<BrowserNode>().title = info.name;
        entity.getComponent<BrowserNode>().action = [&, loadPath]() {m_gameInstance.loadPlugin(loadPath); };
        entity.addComponent<xy::CommandTarget>().ID = CommandID::BrowserNode;

        entity.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), ThumbnailSize };
        entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
                [&, loadPath](xy::Entity e, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse
                        && e.getComponent<BrowserNode>().enabled)
                    {
                        e.getComponent<BrowserNode>().action();
                        LOG("Refactor this into single callback ID!", xy::Logger::Type::Info);
                    }
                });

        auto& parentTx = entity.getComponent<xy::Transform>();

        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(ItemPadding, ItemPadding);
        entity.addComponent<xy::Text>(menuFont).setString(info.name);
        entity.getComponent<xy::Text>().setCharacterSize(24);
        entity.addComponent<xy::Drawable>().setDepth(TextDepth);
        parentTx.addChild(entity.getComponent<xy::Transform>());

        if (!info.version.empty())
        {
            entity = m_scene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Text>(menuFont).setString("Version: " + info.version);
            entity.getComponent<xy::Text>().setCharacterSize(24);
            entity.addComponent<xy::Drawable>().setDepth(TextDepth);

            auto bounds = xy::Text::getLocalBounds(entity);
            entity.getComponent<xy::Transform>().setPosition(ThumbnailSize.x - (bounds.width + ItemPadding), ItemPadding);

            parentTx.addChild(entity.getComponent<xy::Transform>());
        }

        if (!info.author.empty())
        {
            entity = m_scene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Text>(menuFont).setString("Author: " + info.author);
            entity.getComponent<xy::Text>().setCharacterSize(24);
            entity.addComponent<xy::Drawable>().setDepth(TextDepth);

            auto bounds = xy::Text::getLocalBounds(entity);
            entity.getComponent<xy::Transform>().setPosition(ItemPadding, ThumbnailSize.y - (bounds.height + ItemPadding));

            parentTx.addChild(entity.getComponent<xy::Transform>());
        }

        nodePosition.x += ThumbnailSize.x * 1.2f;

        rootNode.addChild(parentTx);
        m_browserTargets.push_back(nodePosition - basePosition);
        m_browserTargets.back().x = -m_browserTargets.back().x;
    }

    //add one extra item to quit the browser
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(nodePosition);
    entity.getComponent<xy::Transform>().setOrigin(ThumbnailSize.x / 2.f, ThumbnailSize.y * 0.8f);
    entity.addComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(TextureID::handles[TextureID::DefaultThumb]));
    applyVertices(entity.getComponent<xy::Drawable>());
    entity.addComponent<BrowserNode>().index = i;
    entity.getComponent<BrowserNode>().title = "Quit";
    entity.getComponent<BrowserNode>().action = []() {xy::App::quit(); };
    entity.addComponent<xy::CommandTarget>().ID = CommandID::BrowserNode;

    entity.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), ThumbnailSize };
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse
                    && e.getComponent<BrowserNode>().enabled)
                {
                    e.getComponent<BrowserNode>().action();
                }
            });

    auto& parentTx = entity.getComponent<xy::Transform>();

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ThumbnailSize / 2.f);
    entity.addComponent<xy::Text>(menuFont).setString("Quit");
    entity.getComponent<xy::Text>().setCharacterSize(64);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);
    parentTx.addChild(entity.getComponent<xy::Transform>());
    rootNode.addChild(parentTx);

    //title text at the bottom
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 320.f);
    entity.addComponent<xy::Text>(menuFont).setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(128);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::TitleText;
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);
}

void BrowserState::buildSlideshow()
{
    //easing function
    auto IOQuad = [](float t)->float
    {
        return (t < 0.5f) ? 2.f * t * t : t * (4.f - 2.f * t) - 1.f;
    };

    const auto backgroundDir = xy::FileSystem::getResourcePath() + "assets/images/backgrounds/";
    auto imageList = xy::FileSystem::listFiles(backgroundDir);
    std::sort(imageList.begin(), imageList.end());

    for (const auto& img : imageList)
    {
        auto tex = std::make_unique<sf::Texture>();
        if (tex->loadFromFile(backgroundDir + img))
        {
            tex->setRepeated(true);
            m_slideshowTextures.push_back(std::move(tex));
        }
    }

    if (!m_slideshowTextures.empty()
        && m_shader.loadFromMemory(SlideshowFrag, sf::Shader::Fragment))
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>();
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Slideshow;
        entity.addComponent<xy::Drawable>().setDepth(BackgroundDepth + 1);
        entity.addComponent<xy::Sprite>(*m_slideshowTextures[0]).setTextureRect({ sf::Vector2f(), xy::DefaultSceneSize });

        m_shader.setUniform("u_texA", *m_slideshowTextures[m_slideshowIndex]);
        m_slideshowIndex = (m_slideshowIndex + 1) % m_slideshowTextures.size();
        m_shader.setUniform("u_texB", *m_slideshowTextures[m_slideshowIndex]);
        m_slideshowIndex = (m_slideshowIndex + 1) % m_slideshowTextures.size();

        entity.getComponent<xy::Drawable>().setShader(&m_shader);
        entity.addComponent<xy::Callback>().active = m_settings.useSlideshow;
        entity.getComponent<xy::Callback>().userData = std::make_any<SlideshowState>();
        entity.getComponent<xy::Callback>().function =
            [&](xy::Entity e, float dt)
        {
            auto& state = std::any_cast<SlideshowState&>(e.getComponent<xy::Callback>().userData);
            
            if (state.state == SlideshowState::In)
            {
                state.easeTime += dt;
                if (state.easeTime > 1)
                {
                    state.easeTime = 1.f;
                    state.state = SlideshowState::Hold;

                    e.getComponent<xy::Drawable>().getShader()->setUniform("u_texA", *m_slideshowTextures[m_slideshowIndex]);
                    m_slideshowIndex = (m_slideshowIndex + 1) % m_slideshowTextures.size();
                }

                e.getComponent<xy::Drawable>().getShader()->setUniform("u_mix", state.easeTime);
            }
            else if (state.state == SlideshowState::Hold)
            {
                if (state.pendingActiveChange)
                {
                    state.pendingActiveChange = false;
                    e.getComponent<xy::Callback>().active = false;
                }

                state.delayTime += dt;

                if (state.delayTime > SlideshowState::HoldTime)
                {
                    state.delayTime = 0.f;

                    state.state = (state.easeTime == 0) ? SlideshowState::In : SlideshowState::Out;
                }
            }
            else if (state.state == SlideshowState::Out)
            {
                state.easeTime -= dt;
                if (state.easeTime < 0)
                {
                    state.easeTime = 0.f;
                    state.state = SlideshowState::Hold;

                    e.getComponent<xy::Drawable>().getShader()->setUniform("u_texB", *m_slideshowTextures[m_slideshowIndex]);
                    m_slideshowIndex = (m_slideshowIndex + 1) % m_slideshowTextures.size();
                }

                e.getComponent<xy::Drawable>().getShader()->setUniform("u_mix", state.easeTime);
            }
        };
    }
    else
    {
        m_slideshowTextures.clear();
    }
}

void BrowserState::nextItem()
{
    m_browserTargetIndex = std::min(m_browserTargetIndex + 1, m_browserTargets.size() - 1);

    xy::Command cmd;
    cmd.targetFlags = CommandID::RootNode;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<Slider>().target = m_browserTargets[m_browserTargetIndex];
        e.getComponent<Slider>().active = true;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::BrowserNode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<BrowserNode>().enabled = false;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    LOG("Play sound here!", xy::Logger::Type::Info);
}

void BrowserState::prevItem()
{
    m_browserTargetIndex = m_browserTargetIndex > 0 ? m_browserTargetIndex - 1 : 0;

    xy::Command cmd;
    cmd.targetFlags = CommandID::RootNode;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<Slider>().target = m_browserTargets[m_browserTargetIndex];
        e.getComponent<Slider>().active = true;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::BrowserNode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<BrowserNode>().enabled = false;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    LOG("Play sound here!", xy::Logger::Type::Info);
}

void BrowserState::execItem()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::BrowserNode;
    cmd.action = [&](xy::Entity e, float)
    {
        auto& node = e.getComponent<BrowserNode>();
        if (node.index == m_browserTargetIndex
            && node.enabled)
        {
            node.action();
        }
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}