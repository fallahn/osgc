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

#include "MenuState.hpp"
#include "StateIDs.hpp"
#include "PluginExport.hpp"
#include "MenuConsts.hpp"
#include "CommandIDs.hpp"
#include "SliderSystem.hpp"
#include "NetConsts.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
#include "MenuShader.inl"
#include "GlobeShader.inl"

    const std::string AppName("space_racers");
    const std::string CfgName("settings.cfg");
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_scene         (ctx.appInstance.getMessageBus()),
    m_activeString  (nullptr)
{
    ctx.appInstance.setMouseCursorVisible(true);

    launchLoadingScreen();

    //make sure any previous sessions are tidied up
    if (sd.netClient)
    {
        sd.netClient->disconnect();
        sd.netClient.reset();
    }

    if (sd.server)
    {
        sd.server->quit();
        sd.server.reset();

        m_sharedData.hosting = false;
    }


    initScene();
    loadResources();
    buildMenu();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    registerCommand("debug_mode",
        [&](const std::string&)
        {
            requestStackClear();
            requestStackPush(StateID::Debug);
        });

    quitLoadingScreen();
}

MenuState::~MenuState()
{
    m_settings.findProperty("name")->setValue(m_sharedData.name.toAnsiString());
    m_settings.findProperty("ip")->setValue(m_sharedData.ip.toAnsiString());
    m_settings.save(xy::FileSystem::getConfigDirectory(AppName) + CfgName);
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    updateTextInput(evt);

    m_scene.getSystem<xy::UISystem>().handleEvent(evt);
    m_scene.forwardEvent(evt);

    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
    static float currTime = 0.f;
    currTime += dt;
    m_shaders.get(ShaderID::Stars).setUniform("u_time", currTime / 100.f);
    m_shaders.get(ShaderID::Globe).setUniform("u_time", currTime / 10.f);

    m_scene.update(dt);
    return true;
}

void MenuState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

xy::StateID MenuState::stateID() const
{
    return StateID::MainMenu;
}

//private
void MenuState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<SliderSystem>(mb);
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
}

void MenuState::loadResources()
{
    FontID::handles[FontID::Default] = m_sharedData.resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");
    
    m_textureIDs[TextureID::Menu::MainMenu] = m_resources.load<sf::Texture>("assets/images/menu_title.png");
    m_textureIDs[TextureID::Menu::Stars] = m_resources.load<sf::Texture>("assets/images/stars.png");

    m_textureIDs[TextureID::Menu::StarsFar] = m_resources.load<sf::Texture>("assets/images/stars_far.png");
    m_textureIDs[TextureID::Menu::StarsMid] = m_resources.load<sf::Texture>("assets/images/stars_mid.png");
    m_textureIDs[TextureID::Menu::StarsNear] = m_resources.load<sf::Texture>("assets/images/stars_near.png");
    m_textureIDs[TextureID::Menu::PlanetDiffuse] = m_resources.load<sf::Texture>("assets/images/globe_diffuse.png");
    m_textureIDs[TextureID::Menu::PlanetNormal] = m_resources.load<sf::Texture>("assets/images/crater_normal.png");

    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::StarsFar]).setRepeated(true);
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::StarsMid]).setRepeated(true);
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::StarsNear]).setRepeated(true);
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::PlanetDiffuse]).setRepeated(true);
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::PlanetNormal]).setRepeated(true);

    m_shaders.preload(ShaderID::Stars, StarsFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Globe, GlobeFragment, sf::Shader::Fragment);

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/menu_buttons.spt", m_resources);
    m_sprites[SpriteID::Menu::TimeTrialButton] = spriteSheet.getSprite("timetrial");
    m_sprites[SpriteID::Menu::LocalButton] = spriteSheet.getSprite("local");
    m_sprites[SpriteID::Menu::NetButton] = spriteSheet.getSprite("net");
    m_sprites[SpriteID::Menu::OptionsButton] = spriteSheet.getSprite("options");
    m_sprites[SpriteID::Menu::QuitButton] = spriteSheet.getSprite("quit");

    spriteSheet.loadFromFile("assets/sprites/network_buttons.spt", m_resources);
    m_sprites[SpriteID::Menu::PlayerNameButton] = spriteSheet.getSprite("player_name");
    m_sprites[SpriteID::Menu::HostButton] = spriteSheet.getSprite("host");
    m_sprites[SpriteID::Menu::AddressButton] = spriteSheet.getSprite("address");
    m_sprites[SpriteID::Menu::JoinButton] = spriteSheet.getSprite("join");
    m_sprites[SpriteID::Menu::NetBackButton] = spriteSheet.getSprite("back");

    if (spriteSheet.loadFromFile("assets/sprites/cursor.spt", m_resources))
    {
        //custom mouse cursor
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>();
        entity.addComponent<xy::Drawable>().setDepth(300);
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("cursor");
        entity.addComponent<xy::SpriteAnimation>().play(0);
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [](xy::Entity e, float)
        {
            auto mousePos = xy::App::getRenderWindow()->mapPixelToCoords(sf::Mouse::getPosition(*xy::App::getRenderWindow()));
            e.getComponent<xy::Transform>().setPosition(mousePos);
        };

        xy::App::getActiveInstance()->setMouseCursorVisible(false);
    }

    if (m_settings.loadFromFile(xy::FileSystem::getConfigDirectory(AppName) + CfgName))
    {
        if (auto prop = m_settings.findProperty("name"); prop)
        {
            m_sharedData.name = prop->getValue<std::string>(); //barnacles. This doesn't support sf::String / unicode
        }
        else
        {
            m_settings.addProperty("name", m_sharedData.name.toAnsiString());
        }

        if (auto prop = m_settings.findProperty("ip"); prop)
        {
            m_sharedData.ip = prop->getValue<std::string>();
        }
        else
        {
            m_settings.addProperty("ip", m_sharedData.ip.toAnsiString());
        }
    }
    else
    {
        m_settings.addProperty("name", m_sharedData.name.toAnsiString());
        m_settings.addProperty("ip", m_sharedData.ip.toAnsiString());
    }
    m_settings.save(xy::FileSystem::getConfigDirectory(AppName) + CfgName);
}

void MenuState::buildMenu()
{
    //background
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::Stars]));

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::BackgroundDepth + 1);
    entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Stars));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.getComponent<xy::Drawable>().bindUniform("u_speed", 1.f);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::StarsFar]));
    entity.getComponent<xy::Sprite>().setTextureRect({ sf::Vector2f(), xy::DefaultSceneSize });

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::BackgroundDepth + 2);
    entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Stars));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.getComponent<xy::Drawable>().bindUniform("u_speed", 2.f);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::StarsMid]));
    entity.getComponent<xy::Sprite>().setTextureRect({ sf::Vector2f(), xy::DefaultSceneSize });

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::BackgroundDepth + 3);
    entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Stars));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.getComponent<xy::Drawable>().bindUniform("u_speed", 4.f);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::StarsNear]));
    entity.getComponent<xy::Sprite>().setTextureRect({ sf::Vector2f(), xy::DefaultSceneSize });

    //planet
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Globe));
    entity.getComponent<xy::Drawable>().setDepth(MenuConst::MenuDepth - 1);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::PlanetNormal]));
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::PlanetDiffuse]));


    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);

    auto rootNode = m_scene.createEntity();
    rootNode.addComponent<xy::Transform>();
    rootNode.addComponent<xy::CommandTarget>().ID = CommandID::RootNode;
    rootNode.addComponent<Slider>().speed = 7.f;

    //title
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::MenuDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::MainMenu]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    auto& parentTx = entity.getComponent<xy::Transform>();
    rootNode.getComponent<xy::Transform>().addChild(parentTx);

    auto& uiSystem = m_scene.getSystem<xy::UISystem>();
    auto mouseEnter = uiSystem.addMouseMoveCallback(
        [](xy::Entity e, sf::Vector2f)
        {
            auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
            bounds.left = bounds.width;
            e.getComponent<xy::Sprite>().setTextureRect(bounds);
        });
    auto mouseExit = uiSystem.addMouseMoveCallback([](xy::Entity e, sf::Vector2f) 
        {
            auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
            bounds.left = 0.f;
            e.getComponent<xy::Sprite>().setTextureRect(bounds);
        });


    sf::Vector2f itemPosition = MenuConst::ItemRootPosition;

    //time trial
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::TimeTrialButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = MenuConst::TimeTrialMenuPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());
    
    itemPosition.y += bounds.height;

    //local
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::LocalButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = MenuConst::LocalPlayMenuPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //net
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NetButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = MenuConst::NetworkMenuPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //options
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::OptionsButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Console::show();
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;


    //quit
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::QuitButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    requestStackClear();
                    requestStackPush(StateID::ParentState);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buildNetworkMenu(rootNode, mouseEnter, mouseExit);
    buildTimeTrialMenu(rootNode, mouseEnter, mouseExit);
    buildLocalPlayMenu(rootNode, mouseEnter, mouseExit);
}

void MenuState::buildNetworkMenu(xy::Entity rootNode, sf::Uint32 mouseEnter, sf::Uint32 mouseExit)
{
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(xy::DefaultSceneSize.x, 0.f);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::MenuDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::MainMenu]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    auto & parentTx = entity.getComponent<xy::Transform>();
    rootNode.getComponent<xy::Transform>().addChild(parentTx);

    auto& uiSystem = m_scene.getSystem<xy::UISystem>();
    sf::Vector2f itemPosition = MenuConst::ItemRootPosition;

    //name input
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::PlayerNameButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_sharedData.name;

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);


                    cmd.targetFlags = CommandID::IPText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);
    auto textEnt = m_scene.createEntity();
    textEnt.addComponent<xy::Transform>().setPosition(26.f, 10.f);
    textEnt.addComponent<xy::Drawable>().setDepth(MenuConst::TextDepth);
    //TODO set drawable cropping area
    textEnt.addComponent<xy::Text>(font).setString(m_sharedData.name);
    textEnt.getComponent<xy::Text>().setCharacterSize(36);
    textEnt.addComponent<xy::CommandTarget>().ID = CommandID::NameText;
    entity.getComponent<xy::Transform>().addChild(textEnt.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //host button
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::HostButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.hosting = true;
                    m_sharedData.ip = "127.0.0.1";
                    requestStackClear();
                    requestStackPush(StateID::Lobby);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //address box
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::AddressButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_sharedData.ip;

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::IPText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    //ip text
    textEnt = m_scene.createEntity();
    textEnt.addComponent<xy::Transform>().setPosition(26.f, 10.f);
    textEnt.addComponent<xy::Drawable>().setDepth(MenuConst::TextDepth);
    //TODO set drawable cropping area
    textEnt.addComponent<xy::Text>(font).setString(m_sharedData.ip);
    textEnt.getComponent<xy::Text>().setCharacterSize(36);
    textEnt.addComponent<xy::CommandTarget>().ID = CommandID::IPText;
    entity.getComponent<xy::Transform>().addChild(textEnt.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //join button
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::JoinButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.hosting = false;
                    requestStackClear();
                    requestStackPush(StateID::Lobby);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //back button
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NetBackButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = MenuConst::MainMenuPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());
}

void MenuState::buildTimeTrialMenu(xy::Entity rootNode, sf::Uint32 mouseEnter, sf::Uint32 mouseExit)
{
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -xy::DefaultSceneSize.y * 0.75f);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NetBackButton];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = MenuConst::MainMenuPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void MenuState::buildLocalPlayMenu(xy::Entity rootNode, sf::Uint32 mouseEnter, sf::Uint32 mouseExit)
{
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, xy::DefaultSceneSize.y * 1.25f);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NetBackButton];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = MenuConst::MainMenuPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void MenuState::updateTextInput(const sf::Event& evt)
{
    if (m_activeString != nullptr)
    {
        std::size_t maxChar = (m_activeString == &m_sharedData.ip)
            ? 15 : NetConst::MaxNameSize / sizeof(sf::Uint32);

        std::uint32_t targetFlags = (m_activeString == &m_sharedData.ip)
            ? CommandID::IPText : CommandID::NameText;

        auto updateText = [&, targetFlags]()
        {
            xy::Command cmd;
            cmd.targetFlags = targetFlags;
            cmd.action = [&](xy::Entity entity, float)
            {
                auto& text = entity.getComponent<xy::Text>();
                text.setString(*m_activeString);
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        };

        if (evt.type == sf::Event::TextEntered)
        {
            //skipping backspace and delete
            if (evt.text.unicode > 31
                /*&& evt.text.unicode < 127*/
                && m_activeString->getSize() < maxChar)
            {
                *m_activeString += evt.text.unicode;
                updateText();
            }
        }
        else if (evt.type == sf::Event::KeyReleased)
        {
            if (evt.key.code == sf::Keyboard::BackSpace
                && !m_activeString->isEmpty())
            {
                m_activeString->erase(m_activeString->getSize() - 1);
                updateText();
            }
            else if (evt.key.code == sf::Keyboard::Return)
            {
                m_activeString = nullptr;

                xy::Command cmd;
                cmd.targetFlags = CommandID::NameText | CommandID::IPText;
                cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
        }
    }
}

void MenuState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}