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
#include "LoadingScreen.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>

#include <xyginext/core/ConfigFile.hpp>
#include <xyginext/core/Console.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/audio/Mixer.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

#include <fstream>
#include <cstring>

namespace SpriteID
{
    std::array<xy::Sprite, SpriteID::Count> sprites = {};
}

namespace TextureID
{
    std::array<std::size_t, TextureID::Count> handles = {};
}

namespace FontID
{
    std::array<std::size_t, Count> handles = {};
}

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

    std::array<sf::Vector2f, 2u> camTargets =
    {
        xy::DefaultSceneSize / 2.f,
        sf::Vector2f(xy::DefaultSceneSize.x / 2.f, xy::DefaultSceneSize.y + (xy::DefaultSceneSize.y / 2.f))
    };

    const std::uint32_t TitleCharSize = 128;
    const sf::Vector2f TitlePosition(960.f, 860.f);

    const std::uint32_t InfoTextSize = 24;

    const std::string settingsFile("browser.cfg");
    const auto ThumbnailSize = xy::DefaultSceneSize / 2.f; //make sure to scale all sprites to correct size

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
    
                gl_FragColor = mix(imgA, imgB, u_mix) * gl_Color;
            }
          )";

    const float FadeInDuration = 2.f;

    struct SlideshowState final
    {
        enum
        {
            In, Hold, Out, Show, Hide
        }state = Show;
        bool pendingActiveChange = false;
        float easeTime = 0.f;
        float delayTime = 0.f;
        float alpha = 0.f;
        static constexpr float HoldTime = 15.f;
    };
}

BrowserState::BrowserState(xy::StateStack& ss, xy::State::Context ctx, Game& game, fe::LoadingScreen& ls)
    : xy::State         (ss, ctx),
    m_gameInstance      (game),
    m_loadingScreen     (ls),
    m_scene             (ctx.appInstance.getMessageBus()),
    m_browserTargetIndex(0),
    m_slideshowIndex    (0),
    m_audioScape        (m_audioResource),
    m_locked            (true),
    m_quitShown         (false)
{
    launchLoadingScreen();
    
    //make sure to unload any active plugin so our asset directory is correct
    game.unloadPlugin();
    
    loadSettings();

    initScene();
    loadResources();

    //set up the camera to switch between views
    m_scene.getActiveCamera().getComponent<xy::Transform>().setPosition(camTargets[m_settings.camTargetIndex]);
    auto& cam = m_scene.getActiveCamera().getComponent<xy::Camera>();
    cam.setView(ctx.defaultView.getSize());
    cam.setViewport(ctx.defaultView.getViewport());

    buildMenu();
    updateTextScene();

    registerConsoleTab("Options",
        [&]() 
        {
            auto oldSlideshow = m_settings.useSlideshow;
            xy::ui::checkbox("Slideshow Background", &m_settings.useSlideshow);
            xy::ui::sameLine(); xy::Nim::showToolTip("Displays images stored in the /images/backgrounds/ directory as a slideshow instead of the selected game background.");
            if (oldSlideshow != m_settings.useSlideshow)
            {
                xy::Command cmd;
                cmd.targetFlags = CommandID::Slideshow;
                cmd.action = [oldSlideshow](xy::Entity e, float)
                {
                    auto& data = std::any_cast<SlideshowState&>(e.getComponent<xy::Callback>().userData);
                    if (oldSlideshow)
                    {
                        data.pendingActiveChange = true;
                    }
                    else
                    {
                        data.state = SlideshowState::Show;
                        e.getComponent<xy::Callback>().active = true;
                    }
                };
                m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                saveSettings();
            };
        });

    registerConsoleTab("About",
        [&]()
        {
            xy::ui::text("OSGC - Open Source Game Collection. Copyright 2019 Matt Marchant and contributors");
            xy::ui::text("For more information visit https://github.com/fallah/osgc");
        });

    ctx.appInstance.setMouseCursorVisible(true);

    if (!m_gameInstance.getArgsString().empty())
    {
        auto path = xy::FileSystem::getResourcePath() + pluginFolder + m_gameInstance.getArgsString() + pluginFile;
        if (xy::FileSystem::fileExists(path))
        {
            path = pluginFolder + m_gameInstance.getArgsString();

            //send as a command to delay by one frame
            //and make sure everything is set up
            xy::Command cmd;
            cmd.targetFlags = CommandID::RootNode;
            cmd.action = [&,path](xy::Entity, float)
            {
                m_gameInstance.loadPlugin(path);
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            m_gameInstance.setArgsString("");
        }
        else
        {
            xy::Logger::log(m_gameInstance.getArgsString() + ": directory not found.", xy::Logger::Type::Error);
        }
    }


#ifdef XY_DEBUG
    m_gameInstance.setWindowTitle("OSGC - The Open Source Game Collection (Debug build) - Press F1 for Console");
#else
    m_gameInstance.setWindowTitle("OSGC - The Open Source Game Collection - Press F1 for Console");
#endif

    quitLoadingScreen();
}

BrowserState::~BrowserState()
{
    //set the default view again
    //so any loading plugin is as it expects
    xy::App::getRenderWindow()->setView(getContext().defaultView);

    saveSettings();
}

bool BrowserState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }
    
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
            m_quitShown ? xy::App::quit() : execItem();
            break;
        case sf::Keyboard::Escape:
            m_quitShown ? hideQuit() : xy::Console::show();
            break;
        case sf::Keyboard::BackSpace:
            if (m_quitShown)
            {
                hideQuit();
            }
            break;
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed)
    {
        //TODO controller mappings.
        switch (evt.joystickButton.button)
        {
        default: break;
        case 0:
            m_quitShown ? xy::App::quit() : execItem();
            break;
        case 1: 
            if (m_quitShown)
            {
                hideQuit();
            }
            break;
        case 4:
            prevItem();
            break;
        case 5:
            nextItem();
            break;
        case 7:
            m_quitShown ? hideQuit() : xy::Console::show();
            break;
        }
    }
    else if (evt.type == sf::Event::JoystickMoved)
    {
        if (evt.joystickMove.axis == sf::Joystick::PovX
            /*|| evt.joystickMove.axis == sf::Joystick::X*/)
        {
            if (evt.joystickMove.position < -50)
            {
                prevItem();
            }
            else if(evt.joystickMove.position > 50)
            {
                nextItem();
            }
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
            enableItem();
        }
    }

    else if (msg.id == xy::Message::WindowMessage)
    {
        const auto& data = msg.getData<xy::Message::WindowEvent>();
        if (data.type == xy::Message::WindowEvent::Resized)
        {
            updateTextScene();
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
    m_scene.addSystem<fe::SliderSystem>(messageBus);
    m_scene.addSystem<xy::CallbackSystem>(messageBus);
    m_scene.addSystem<xy::TextSystem>(messageBus);
    m_scene.addSystem<xy::UISystem>(messageBus).setJoypadCursorActive(false);
    m_scene.addSystem<xy::SpriteSystem>(messageBus);

    m_scene.addSystem<xy::CameraSystem>(messageBus);
    m_scene.addSystem<xy::RenderSystem>(messageBus);
    m_scene.addSystem<xy::AudioSystem>(messageBus);

    //set up the mixer channels - these may have been altered by plugins
    xy::AudioMixer::setLabel("Music", AudioChannel::Music);
    xy::AudioMixer::setLabel("Effects", AudioChannel::Effects);

    for (std::uint8_t i = AudioChannel::Count; i < xy::AudioMixer::MaxChannels; ++i)
    {
        xy::AudioMixer::setLabel("Channel " + std::to_string(i), i);
    }
}

void BrowserState::loadResources()
{
    //load resources
    FontID::handles[FontID::MenuFont] = m_resources.load<sf::Font>("assets/fonts/Jellee-Roman.otf");
    FontID::handles[FontID::TitleFont] = m_resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");
    
    TextureID::handles[TextureID::Fallback] = m_resources.load<sf::Texture>("fallback");
    TextureID::handles[TextureID::Background] = m_resources.load<sf::Texture>("assets/images/background.png");
    TextureID::handles[TextureID::BackgroundShine] = m_resources.load<sf::Texture>("assets/images/shine.png");
    TextureID::handles[TextureID::DefaultThumb] = m_resources.load<sf::Texture>("assets/images/default_thumb.png");
    TextureID::handles[TextureID::Quit] = m_resources.load<sf::Texture>("assets/images/quit.png");

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/ui.spt", m_resources);
    SpriteID::sprites[SpriteID::Arrow] = spriteSheet.getSprite("nav_arrow");
    SpriteID::sprites[SpriteID::Options] = spriteSheet.getSprite("options");
    SpriteID::sprites[SpriteID::Quit] = spriteSheet.getSprite("close");
    SpriteID::sprites[SpriteID::Yes] = spriteSheet.getSprite("yes");
    SpriteID::sprites[SpriteID::No] = spriteSheet.getSprite("no");
    SpriteID::sprites[SpriteID::Sort] = spriteSheet.getSprite("sort");
    SpriteID::sprites[SpriteID::Tiled] = spriteSheet.getSprite("tile");

    m_audioScape.loadFromFile("assets/sound/ui.xas");

    xy::App::getActiveInstance()->setWindowIcon("assets/images/o.png");
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

    auto& camTx = m_scene.getActiveCamera().getComponent<xy::Transform>();
    
    //background image is independent of slideshow and visible when
    //slideshow mode is disabled.
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Background]));
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

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
    entity.addComponent<xy::Text>(m_resources.get<sf::Font>(FontID::handles[FontID::TitleFont])).setString("OSGC");
    entity.getComponent<xy::Text>().setCharacterSize(48);
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    //tile mode toggle
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Tiled];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureRect();
    bounds.top = bounds.height * m_settings.camTargetIndex;
    entity.getComponent<xy::Sprite>().setTextureRect(bounds);
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - ((bounds.width + ItemPadding) * 4.f), ItemPadding);
    entity.addComponent<UINode>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UINode;
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse
                    && e.getComponent<UINode>().enabled)
                {
                    m_settings.camTargetIndex = (m_settings.camTargetIndex + 1) % camTargets.size();
                    m_scene.getActiveCamera().getComponent<xy::Transform>().setPosition(camTargets[m_settings.camTargetIndex]);

                    auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
                    bounds.top = bounds.height * m_settings.camTargetIndex;
                    e.getComponent<xy::Sprite>().setTextureRect(bounds);
                }
            });
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    //sort icon
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Sort];
    auto sortBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - ((sortBounds.width + ItemPadding) * 3.f), ItemPadding);
    entity.addComponent<UINode>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UINode;
    entity.addComponent<xy::UIHitBox>().area = sortBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity e, sf::Uint64 flags) 
            {
                if(flags & xy::UISystem::LeftMouse
                    && e.getComponent<UINode>().enabled)
                {
                    sortNodes(!m_settings.lastSort);
                    enableItem();
                }
            });
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    //options icon
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Options];
    auto optionBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - ((optionBounds.width + ItemPadding) * 2.f), ItemPadding);
    entity.addComponent<UINode>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UINode;
    entity.addComponent<xy::UIHitBox>().area = optionBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse
                    && e.getComponent<UINode>().enabled)
                {
                    xy::Console::show();
                }
            });
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    //quit icon
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Quit];
    auto quitBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - (optionBounds.width + ItemPadding), ItemPadding);
    entity.addComponent<UINode>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UINode;
    entity.addComponent<xy::UIHitBox>().area = quitBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse
                    && e.getComponent<UINode>().enabled)
                {
                    showQuit();
                }
            });
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    //navigation arrows
    entity = m_scene.createEntity();    
    entity.addComponent<xy::Drawable>().setDepth(ArrowDepth);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Arrow];
    auto arrowBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
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
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Arrow];
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
    entity.addComponent<RootNode>();
    entity.addComponent<fe::Slider>().speed = 15.f;
    //entity.getComponent<Slider>().active = true; //ensures first node is selected. Now done by fade out callback, below
    entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("navigate");
    entity.getComponent<xy::AudioEmitter>().setChannel(AudioChannel::Effects);

    //game plugin nodes
    sf::Vector2f nodePosition = xy::DefaultSceneSize / 2.f;
    nodePosition.y += (ThumbnailSize.y * 0.6f) / 2.f;
    m_basePosition = nodePosition;
    m_browserTargets.emplace_back();

    //callback when clicking node
    auto nodeCallback = m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
        [](xy::Entity e, sf::Uint64 flags)
        {
            if (flags & xy::UISystem::LeftMouse
                && e.getComponent<BrowserNode>().enabled)
            {
                e.getComponent<BrowserNode>().action();
            }
        });

    auto tileButtonCallback = m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
        [](xy::Entity e, sf::Uint64 flags)
        {
            if (flags & xy::UISystem::LeftMouse
                && e.getComponent<TileNode>().enabled)
            {
                e.getComponent<TileNode>().action();
            }
        });

    auto tileMouseInCallback = m_scene.getSystem<xy::UISystem>().addMouseMoveCallback(
        [&](xy::Entity e, sf::Vector2f)
        {
            if (e.getComponent<TileNode>().enabled)
            {
                xy::Command cmd;
                cmd.targetFlags = CommandID::TileTitleText;
                cmd.action = [e](xy::Entity titleEnt, float)
                {
                    titleEnt.getComponent<xy::Text>().setString(e.getComponent<TileNode>().title);
                };
                m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
        });

    /*auto tileMouseOutCallback = m_scene.getSystem<xy::UISystem>().addMouseMoveCallback(
        [&](xy::Entity e, sf::Vector2f)
        {
            if (e.getComponent<TileNode>().enabled)
            {
                xy::Command cmd;
                cmd.targetFlags = CommandID::TileTitleText;
                cmd.action = [](xy::Entity titleEnt, float)
                {
                    titleEnt.getComponent<xy::Text>().setString("OSGC");
                };
                m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
        });*/

    std::size_t i = 0;
    auto tileNodeParent = m_scene.createEntity();
    tileNodeParent.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y);

    auto pluginList = xy::FileSystem::listDirectories(xy::FileSystem::getResourcePath() + pluginFolder);
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

        m_resources.get<sf::Texture>(thumbID).setSmooth(true);

        //create entity
        auto loadPath = pluginFolder + dir;

        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(nodePosition);
        entity.getComponent<xy::Transform>().setOrigin(ThumbnailSize.x / 2.f, ThumbnailSize.y * 0.8f);
        entity.addComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(thumbID));
        applyVertices(entity.getComponent<xy::Drawable>());
        entity.addComponent<BrowserNode>().index = i;
        entity.getComponent<BrowserNode>().title = info.name;
        entity.getComponent<BrowserNode>().action = [&, loadPath]() {m_gameInstance.loadPlugin(loadPath); };
        entity.addComponent<xy::CommandTarget>().ID = CommandID::BrowserNode;

        entity.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), ThumbnailSize };
        entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] = nodeCallback;

        //store node entity so we can sort by name
        m_nodeList.emplace_back(std::make_pair(entity, info.name));

        //create a smaller version for tile view
        auto tileEnt = addTileNode(i, thumbID, loadPath, info);
        tileNodeParent.getComponent<xy::Transform>().addChild(tileEnt.getComponent<xy::Transform>());
        tileEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), ThumbnailSize };
        tileEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] = tileButtonCallback;
        //tileEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = tileMouseOutCallback;
        tileEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = tileMouseInCallback;

        auto& parentTx = entity.getComponent<xy::Transform>();

        if (!info.version.empty())
        {
            entity = m_scene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Text>(menuFont).setString("Version: " + info.version);
            entity.getComponent<xy::Text>().setCharacterSize(InfoTextSize);
            entity.getComponent<xy::Text>().setOutlineThickness(1.f);
            //entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
            entity.addComponent<xy::Drawable>().setDepth(TextDepth);
            entity.addComponent<xy::CommandTarget>().ID = CommandID::ScaledText;

            auto bounds = xy::Text::getLocalBounds(entity);
            entity.getComponent<xy::Transform>().setPosition(ThumbnailSize.x - (bounds.width + (ItemPadding * 4.f)), ThumbnailSize.y - (bounds.height + ItemPadding));

            parentTx.addChild(entity.getComponent<xy::Transform>());
        }

        if (!info.author.empty())
        {
            entity = m_scene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Text>(menuFont).setString("Author: " + info.author);
            entity.getComponent<xy::Text>().setCharacterSize(InfoTextSize);
            entity.getComponent<xy::Text>().setOutlineThickness(1.f);
            //entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
            entity.addComponent<xy::Drawable>().setDepth(TextDepth);
            entity.addComponent<xy::CommandTarget>().ID = CommandID::ScaledText;

            auto bounds = xy::Text::getLocalBounds(entity);
            entity.getComponent<xy::Transform>().setPosition(ItemPadding * 4.f, ThumbnailSize.y - (bounds.height + ItemPadding));

            parentTx.addChild(entity.getComponent<xy::Transform>());
        }

        nodePosition.x += ThumbnailSize.x * 1.2f;

        rootNode.addChild(parentTx);
        m_browserTargets.push_back(nodePosition - m_basePosition);
        m_browserTargets.back().x = -m_browserTargets.back().x;

        i++;
    }

    //sort nodes alphabetically. Quit node (below) is ignored so it is always last
    sortNodes(m_settings.lastSort);

    //add one extra item to quit the browser
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(nodePosition);
    entity.getComponent<xy::Transform>().setOrigin(ThumbnailSize.x / 2.f, ThumbnailSize.y * 0.8f);
    entity.addComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(TextureID::handles[TextureID::Quit]));
    applyVertices(entity.getComponent<xy::Drawable>());
    entity.addComponent<BrowserNode>().index = i;
    entity.getComponent<BrowserNode>().title = "Quit";
    entity.getComponent<BrowserNode>().action = [&]() { showQuit(); };
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

    /*entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ThumbnailSize / 2.f);
    entity.addComponent<xy::Text>(menuFont).setString("Quit");
    entity.getComponent<xy::Text>().setCharacterSize(64);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);
    parentTx.addChild(entity.getComponent<xy::Transform>());*/
    rootNode.addChild(parentTx);

    //title text at the bottom
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(TitlePosition);
    entity.addComponent<xy::Text>(menuFont).setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(TitleCharSize);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setOutlineColour(sf::Color::White);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::TitleText;
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);

    //tile version
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(TitlePosition);
    entity.getComponent<xy::Transform>().move(0.f, xy::DefaultSceneSize.y);
    entity.addComponent<xy::Text>(menuFont).setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(TitleCharSize);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setOutlineColour(sf::Color::White);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setString("OSGC");
    entity.addComponent<xy::CommandTarget>().ID = CommandID::TileTitleText;
    entity.addComponent<xy::Drawable>().setDepth(TextDepth);

    //create a fade in which also plays the intro sound
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(1000);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.resize(4);
    verts[0].color = sf::Color::Black;
    verts[1] = { sf::Vector2f(xy::DefaultSceneSize.x, 0.f), sf::Color::Black };
    verts[2] = { sf::Vector2f(xy::DefaultSceneSize. x, xy::DefaultSceneSize.y * 2.f), sf::Color::Black };
    verts[3] = { sf::Vector2f(0.f, xy::DefaultSceneSize.y * 2.f), sf::Color::Black };
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.getComponent<xy::Drawable>().setPrimitiveType(sf::Quads);

    entity.addComponent<xy::AudioEmitter>().setSource("assets/sound/startup.wav");
    entity.getComponent<xy::AudioEmitter>().setChannel(AudioChannel::Effects);
    entity.getComponent<xy::AudioEmitter>().play();

    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().userData = std::make_any<float>(FadeInDuration);
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float dt)
    {
        float& amount = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        auto oldAmount = amount;
        amount = std::max(0.f, amount - dt);

        sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * (amount / FadeInDuration));
        auto& verts = e.getComponent<xy::Drawable>().getVertices();
        for (auto& v : verts) v.color = { 0,0,0,alpha };

        if (e.getComponent<xy::AudioEmitter>().getStatus() == xy::AudioEmitter::Stopped)
        {
            m_scene.destroyEntity(e);
        }

        //enable root node once alpha is zero to enable menu
        if (amount == 0 && amount < oldAmount)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::RootNode;
            cmd.action = [](xy::Entity f, float)
            {
                f.getComponent<fe::Slider>().active = true;
                f.getComponent<RootNode>().enabled = true;
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = CommandID::TileNode;
            cmd.action = [](xy::Entity f, float)
            {
                f.getComponent<TileNode>().enabled = true;
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            m_locked = false;
        }
    };
}

void BrowserState::buildSlideshow()
{
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
        entity.getComponent<xy::Sprite>().setColour({ 255,255,255,0 });

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
            //easing function
            auto IOQuad = [](float t)->float
            {
                return (t < 0.5f) ? 2.f * t * t : t * (4.f - 2.f * t) - 1.f;
            };
            
            auto& state = std::any_cast<SlideshowState&>(e.getComponent<xy::Callback>().userData);
            switch (state.state)
            {
            default: break;
            case SlideshowState::In:
            {
                state.easeTime += dt;
                if (state.easeTime > 1)
                {
                    state.easeTime = 1.f;
                    state.state = SlideshowState::Hold;

                    e.getComponent<xy::Drawable>().getShader()->setUniform("u_texA", *m_slideshowTextures[m_slideshowIndex]);
                    m_slideshowIndex = (m_slideshowIndex + 1) % m_slideshowTextures.size();
                }

                e.getComponent<xy::Drawable>().getShader()->setUniform("u_mix", IOQuad(state.easeTime));
            }
                break;
            case SlideshowState::Hold:
            {
                if (state.pendingActiveChange)
                {
                    state.pendingActiveChange = false;
                    state.state = SlideshowState::Hide;
                }
                else
                {
                    state.delayTime += dt;

                    if (state.delayTime > SlideshowState::HoldTime)
                    {
                        state.delayTime = 0.f;

                        state.state = (state.easeTime == 0) ? SlideshowState::In : SlideshowState::Out;
                    }
                }
            }
                break;
            case SlideshowState::Out:
            {
                state.easeTime -= dt;
                if (state.easeTime < 0)
                {
                    state.easeTime = 0.f;
                    state.state = SlideshowState::Hold;

                    e.getComponent<xy::Drawable>().getShader()->setUniform("u_texB", *m_slideshowTextures[m_slideshowIndex]);
                    m_slideshowIndex = (m_slideshowIndex + 1) % m_slideshowTextures.size();
                }

                e.getComponent<xy::Drawable>().getShader()->setUniform("u_mix", IOQuad(state.easeTime));
            }
                break;
            case SlideshowState::Show:
                state.alpha += dt;
                if (state.alpha > 1.f)
                {
                    state.alpha = 1.f;
                    state.state = SlideshowState::Hold;
                }

                e.getComponent<xy::Sprite>().setColour({ 255, 255, 255, static_cast<sf::Uint8>(state.alpha * 255.f) });
                break;
            case SlideshowState::Hide:
                state.alpha -= dt;
                if (state.alpha < 0.f)
                {
                    state.alpha = 0.f;
                    state.state = SlideshowState::Hold;
                    e.getComponent<xy::Callback>().active = false;
                }

                e.getComponent<xy::Sprite>().setColour({ 255, 255, 255, static_cast<sf::Uint8>(state.alpha * 255.f) });
                break;
            }
        };

        entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
        auto& camTx = m_scene.getActiveCamera().getComponent<xy::Transform>();
        camTx.addChild(entity.getComponent<xy::Transform>());
    }
    else
    {
        m_slideshowTextures.clear();
    }
}

xy::Entity BrowserState::addTileNode(std::size_t index, std::size_t textureID, const std::string& loadPath, const PluginInfo& info)
{
    //TODO we need to paginate these when there are more plugins than can fit on screen.

    const std::size_t TilesPerRow = 7;
    const float ThumbWidth = xy::DefaultSceneSize.x / 8.f;
    const float Padding = (xy::DefaultSceneSize.x - (TilesPerRow * ThumbWidth)) / 8.f;
    const float RowHeight = (xy::DefaultSceneSize.y / 8.f);

    sf::Vector2f nodePosition((ThumbWidth + Padding) * (index % TilesPerRow), (RowHeight + Padding) * (index / TilesPerRow));
    nodePosition += sf::Vector2f(Padding, Padding + 96.f);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(nodePosition);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(textureID));
    entity.addComponent<TileNode>().title = info.name;
    entity.getComponent<TileNode>().author = info.author;
    entity.getComponent<TileNode>().version = info.version;
    entity.getComponent<TileNode>().enabled = false;
    entity.getComponent<TileNode>().action = [&, loadPath]() {m_gameInstance.loadPlugin(loadPath); };
    entity.addComponent<xy::CommandTarget>().ID = CommandID::TileNode;

    m_tileList.emplace_back(std::make_pair(entity, info.name));
    m_tilePositions.emplace_back(nodePosition);

    return entity;
}

void BrowserState::nextItem()
{
    if (m_locked)
    {
        return;
    }

    auto oldIdx = m_browserTargetIndex;
    m_browserTargetIndex = std::min(m_browserTargetIndex + 1, m_browserTargets.size() - 1);

    if (oldIdx != m_browserTargetIndex)
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::RootNode;
        cmd.action = [&](xy::Entity e, float)
        {
            if (e.getComponent<RootNode>().enabled)
            {
                e.getComponent<fe::Slider>().target = m_browserTargets[m_browserTargetIndex];
                e.getComponent<fe::Slider>().active = true;
                //e.getComponent<xy::AudioEmitter>().stop();
                e.getComponent<xy::AudioEmitter>().play();
            }
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = CommandID::BrowserNode;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<BrowserNode>().enabled = false;
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void BrowserState::prevItem()
{
    if (m_locked)
    {
        return;
    }

    auto oldIdx = m_browserTargetIndex;
    m_browserTargetIndex = m_browserTargetIndex > 0 ? m_browserTargetIndex - 1 : 0;

    if (oldIdx != m_browserTargetIndex)
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::RootNode;
        cmd.action = [&](xy::Entity e, float)
        {
            if (e.getComponent<RootNode>().enabled)
            {
                e.getComponent<fe::Slider>().target = m_browserTargets[m_browserTargetIndex];
                e.getComponent<fe::Slider>().active = true;
                //e.getComponent<xy::AudioEmitter>().stop();
                e.getComponent<xy::AudioEmitter>().play();
            }
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = CommandID::BrowserNode;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<BrowserNode>().enabled = false;
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void BrowserState::execItem()
{
    if (m_locked)
    {
        return;
    }

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

void BrowserState::enableItem()
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
}

void BrowserState::showQuit()
{
    if (m_locked)
    {
        return;
    }
    
    //disable all input
    xy::Command cmd;
    cmd.targetFlags = CommandID::RootNode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<RootNode>().enabled = false;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::BrowserNode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<BrowserNode>().enabled = false;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::UINode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<UINode>().enabled = false;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::TileNode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<TileNode>().enabled = false;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //show confirmation
    int renderDepth = 1000;
    auto& camTx = m_scene.getActiveCamera().getComponent<xy::Transform>();
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ConfirmationEntity;
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(renderDepth);

    sf::Color c(0, 0, 0, 0);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.resize(4);
    verts[0].color = c;
    verts[1] = { sf::Vector2f(xy::DefaultSceneSize.x, 0.f), c };
    verts[2] = { xy::DefaultSceneSize, c };
    verts[3] = { sf::Vector2f(0.f, xy::DefaultSceneSize.y), c };
    entity.getComponent<xy::Drawable>().updateLocalBounds();

    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().userData = std::make_any<std::pair<float, float>>(std::make_pair(0.f, 200.f));
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float dt)
    {
        auto& [current, maxVal] = std::any_cast<std::pair<float, float>&>(e.getComponent<xy::Callback>().userData);

        current = std::min(1.f, current + (dt * 8.f));
        sf::Uint8 alpha = static_cast<sf::Uint8>(maxVal * current);

        auto& verts = e.getComponent<xy::Drawable>().getVertices();
        for (auto& v : verts)
        {
            v.color.a = alpha;
        }

        if (current == 1)
        {
            e.getComponent<xy::Callback>().active = false;
        }
    };
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ConfirmationEntity;
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -80.f);
    entity.addComponent<xy::Drawable>().setDepth(renderDepth + 1);
    entity.addComponent<xy::Text>(m_resources.get<sf::Font>(FontID::handles[FontID::MenuFont]));
    entity.getComponent<xy::Text>().setString("Really Quit?");
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setOutlineColour(sf::Color::White);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(120);
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    //yes button
    entity = m_scene.createEntity();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ConfirmationEntity;
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(renderDepth + 1);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Yes];
    auto spriteBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(spriteBounds.width / 2.f, spriteBounds.height / 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(-spriteBounds.width * 2.f, spriteBounds.height * 2.f);
    entity.addComponent<xy::UIHitBox>().area = spriteBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [](xy::Entity, sf::Uint64 flags) 
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::App::quit();
                }
            });
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    //no button
    entity = m_scene.createEntity();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ConfirmationEntity;
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(renderDepth + 1);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::No];
    spriteBounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(spriteBounds.width / 2.f, spriteBounds.height / 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(spriteBounds.width * 2.f, spriteBounds.height * 2.f);
    entity.addComponent<xy::UIHitBox>().area = spriteBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    hideQuit();
                }
            });
    entity.getComponent<xy::Transform>().move(-xy::DefaultSceneSize / 2.f);
    camTx.addChild(entity.getComponent<xy::Transform>());

    m_quitShown = true;
}

void BrowserState::hideQuit()
{
    //reenable UI
    xy::Command cmd;
    cmd.targetFlags = CommandID::RootNode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<RootNode>().enabled = true;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::BrowserNode;
    cmd.action = [&](xy::Entity e, float)
    {
        if (e.getComponent<BrowserNode>().index == m_browserTargetIndex)
        {
            e.getComponent<BrowserNode>().enabled = true;
        }
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::UINode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<UINode>().enabled = true;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::TileNode;
    cmd.action = [](xy::Entity e, float)
    {
        e.getComponent<TileNode>().enabled = true;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //destroy the confirmation entities
    cmd.targetFlags = CommandID::ConfirmationEntity;
    cmd.action = [&](xy::Entity e, float)
    {
        if (e.hasComponent<xy::Callback>())
        {
            e.getComponent<xy::Callback>().function =
                [&](xy::Entity ent, float dt)
            {
                auto& [current, maxVal] = std::any_cast<std::pair<float, float>&>(ent.getComponent<xy::Callback>().userData);

                current = std::max(0.f, current - (dt * 8.f));
                sf::Uint8 alpha = static_cast<sf::Uint8>(maxVal * current);

                auto & verts = ent.getComponent<xy::Drawable>().getVertices();
                for (auto& v : verts)
                {
                    v.color.a = alpha;
                }

                if (current == 0)
                {
                    m_scene.destroyEntity(ent);
                }
            };
            e.getComponent<xy::Callback>().active = true;
        }
        else
        {
            m_scene.destroyEntity(e);
        }
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    m_quitShown = false;
}

void BrowserState::sortNodes(bool asc)
{
    if (asc)
    {
        std::sort(m_nodeList.begin(), m_nodeList.end(),
            [](const std::pair<xy::Entity, std::string>& a, const std::pair<xy::Entity, std::string>& b)
            {
                return a.second < b.second;
            });

        std::sort(m_tileList.begin(), m_tileList.end(),
            [](const std::pair<xy::Entity, std::string>& a, const std::pair<xy::Entity, std::string>& b)
            {
                return a.second < b.second;
            });
    }
    else
    {
        std::sort(m_nodeList.begin(), m_nodeList.end(),
            [](const std::pair<xy::Entity, std::string> & a, const std::pair<xy::Entity, std::string> & b)
            {
                return a.second > b.second;
            });

        std::sort(m_tileList.begin(), m_tileList.end(),
            [](const std::pair<xy::Entity, std::string>& a, const std::pair<xy::Entity, std::string>& b)
            {
                return a.second > b.second;
            });
    }
    m_settings.lastSort = asc;

    auto nodePos = m_basePosition;
    auto stride = ThumbnailSize.x * 1.2f;

    for (auto i = 0u; i < m_nodeList.size(); ++i)
    {
        m_nodeList[i].first.getComponent<xy::Transform>().setPosition(nodePos);
        m_nodeList[i].first.getComponent<BrowserNode>().index = i;
        nodePos.x += stride;

        m_tileList[i].first.getComponent<xy::Transform>().setPosition(m_tilePositions[i]);
    }
}

void BrowserState::updateTextScene()
{
    static float windowScale = 1.f;

    auto currView = xy::App::getRenderWindow()->getDefaultView();
    windowScale = currView.getSize().x / xy::DefaultSceneSize.x;

    xy::Command cmd;
    cmd.targetFlags = CommandID::TitleText | CommandID::TileTitleText;
    cmd.action = [](xy::Entity e, float)
    {
        //e.getComponent<xy::Transform>().setPosition(TitlePosition * windowScale);

        auto charSize = static_cast<std::uint32_t>(static_cast<float>(TitleCharSize) * windowScale);
        e.getComponent<xy::Text>().setCharacterSize(charSize);

        auto scale = 1.f / windowScale;
        e.getComponent<xy::Transform>().setScale(scale, scale);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::ScaledText;
    cmd.action = [](xy::Entity e, float)
    {
        auto charSize = static_cast<std::uint32_t>(static_cast<float>(InfoTextSize) * windowScale);
        e.getComponent<xy::Text>().setCharacterSize(charSize);

        auto scale = 1.f / windowScale;
        e.getComponent<xy::Transform>().setScale(scale, scale);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void BrowserState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_loadingScreen.update(dt);
    rw.draw(m_loadingScreen);
}
