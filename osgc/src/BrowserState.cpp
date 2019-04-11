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

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <xyginext/core/ConfigFile.hpp>
#include <xyginext/core/Console.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

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
}

BrowserState::BrowserState(xy::StateStack& ss, xy::State::Context ctx, Game& game)
    : xy::State         (ss, ctx),
    m_gameInstance      (game),
    m_scene             (ctx.appInstance.getMessageBus()),
    m_browserTargetIndex(0)
{
    launchLoadingScreen();
    
    //make sure to unload any active plugin so our asset directory is correct
    game.unloadPlugin();
    
    initScene();
    loadResources();
    buildMenu();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    ctx.appInstance.setMouseCursorVisible(true);

    quitLoadingScreen();
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

void BrowserState::initScene()
{
    //add the systems
    auto& messageBus = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(messageBus);
    m_scene.addSystem<BrowserNodeSystem>(messageBus);
    m_scene.addSystem<SliderSystem>(messageBus);
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
    TextureID::handles[TextureID::DefaultThumb] = m_resources.load<sf::Texture>("assets/images/default_thumb.png");
    TextureID::handles[TextureID::Arrow] = m_resources.load<sf::Texture>("assets/images/navigation.png");
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

    static const int BackgroundDepth = -20;
    static const int ArrowDepth = -10;
    static const int TextDepth = 1;

    //background
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Background]));

    auto & menuFont = m_resources.get<sf::Font>(FontID::handles[FontID::MenuFont]);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ItemPadding, ItemPadding);
    entity.addComponent<xy::Text>(menuFont).setString("OSGC");
    entity.getComponent<xy::Text>().setCharacterSize(80);
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);

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