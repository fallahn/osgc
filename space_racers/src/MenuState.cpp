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
#include "VehicleSelectSystem.hpp"
#include "GameConsts.hpp"
#include "DigitSystem.hpp"

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
#include <xyginext/graphics/postprocess/ChromeAb.hpp>
#include <xyginext/util/Math.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
#include "MenuShader.inl"
#include "GlobeShader.inl"
#include "MonitorShader.inl"

    const std::string AppName("space_racers");
    const std::string CfgName("settings.cfg");
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_scene             (ctx.appInstance.getMessageBus()),
    m_activeString      (nullptr),
    m_mapIndex          (0),
    m_eliminationMode   (true)
{
    ctx.appInstance.setMouseCursorVisible(true);

    launchLoadingScreen();

    //make sure any previous sessions are tidied up
    m_sharedData.playerInfo.clear();
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
    m_shaders.get(ShaderID::MonitorScreen).setUniform("u_time", currTime / 100.f);

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
    m_scene.addSystem<VehicleSelectSystem>(mb);
    m_scene.addSystem<DigitSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    m_scene.addPostProcess<xy::PostChromeAb>();
}

void MenuState::loadResources()
{
    m_sharedData.fontID = m_sharedData.resources.load<sf::Font>("assets/fonts/" + FontID::DefaultFont);
    
    m_textureIDs[TextureID::Menu::MainMenu] = m_resources.load<sf::Texture>("assets/images/menu_title.png");
    m_textureIDs[TextureID::Menu::Stars] = m_resources.load<sf::Texture>("assets/images/stars.png");
    m_textureIDs[TextureID::Menu::VehicleSelect] = m_resources.load<sf::Texture>("assets/images/vehicle_select_large.png");
    m_textureIDs[TextureID::Menu::MenuBackground] = m_resources.load<sf::Texture>("assets/images/player_select.png");
    m_textureIDs[TextureID::Menu::TrackSelect] = m_resources.load<sf::Texture>("assets/images/track_select.png");
    m_textureIDs[TextureID::Menu::LapCounter] = m_resources.load<sf::Texture>("assets/images/counter.png");
    m_textureIDs[TextureID::Menu::LapFrame] = m_resources.load<sf::Texture>("assets/images/lap_selector.png");
    m_textureIDs[TextureID::Menu::LightBar] = m_resources.load<sf::Texture>("assets/images/lightbar.png");

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
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::LapCounter]).setRepeated(true);

    m_shaders.preload(ShaderID::Stars, StarsFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Globe, GlobeFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::MonitorScreen, MonitorFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Lightbar, LightbarFragment, sf::Shader::Fragment);

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

    spriteSheet.loadFromFile("assets/sprites/menu_arrows.spt", m_resources);
    m_sprites[SpriteID::Menu::NavLeft] = spriteSheet.getSprite("left");
    m_sprites[SpriteID::Menu::NavRight] = spriteSheet.getSprite("right");

    spriteSheet.loadFromFile("assets/sprites/menu_toggle.spt", m_resources);
    m_sprites[SpriteID::Menu::Toggle] = spriteSheet.getSprite("toggle");

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

    //load map names and map them to the thumb texture IDs
    auto mapNames = xy::FileSystem::listFiles(xy::FileSystem::getResourcePath() + "assets/maps");
    mapNames.erase(std::remove_if(mapNames.begin(), mapNames.end(),
        [](const std::string& str)
        {
            return xy::FileSystem::getFileExtension(str) != ".tmx";
        }), mapNames.end());

    auto defaultThumb = m_resources.load<sf::Texture>("assets/images/thumbs/None.png");
    for (const auto& name : mapNames)
    {
        auto str = name.substr(0, name.find(".tmx"));
        if (xy::FileSystem::fileExists(xy::FileSystem::getResourcePath() + "assets/images/thumbs/" + str + ".png"))
        {
            m_mapInfo.emplace_back(std::make_pair(name, m_resources.load<sf::Texture>("assets/images/thumbs/" + str + ".png")));
        }
        else
        {
            m_mapInfo.emplace_back(std::make_pair(name, defaultThumb));
        }
    }
    if (m_mapInfo.empty())
    {
        m_sharedData.errorMessage = "No maps were found!";
        requestStackPush(StateID::Error);
    }
    else
    {
        m_sharedData.mapName = m_mapInfo[m_mapIndex].first;
    }
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


    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);

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

    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);
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
    auto& uiSystem = m_scene.getSystem<xy::UISystem>();

    //back button
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuConst::NavLeftPosition);
    entity.getComponent<xy::Transform>().move(0.f, -xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NavLeft];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
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
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //next button
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuConst::NavRightPosition);
    entity.getComponent<xy::Transform>().move(0.f, -xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NavRight];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    requestStackClear();
                    requestStackPush(StateID::TimeTrial);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //background
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::MenuBackground]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //map select
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -xy::DefaultSceneSize.y * 0.88f);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::TrackSelect]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::ThumbnailPosition);
    thumbEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 1);
    thumbEnt.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::MonitorScreen));
    thumbEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    thumbEnt.getComponent<xy::Drawable>().setBlendMode(sf::BlendMultiply);
    thumbEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_mapInfo[m_mapIndex].second));
    thumbEnt.addComponent<xy::CommandTarget>().ID = CommandID::Menu::TrackThumb;
    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());

    thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::PrevTrackPosition);
    thumbEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::TrackButtonSize };
    thumbEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_mapIndex = (m_mapIndex + (m_mapInfo.size() - 1)) % m_mapInfo.size();
                    m_sharedData.mapName = m_mapInfo[m_mapIndex].first;

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::TrackThumb;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<xy::Sprite>().setTexture(m_resources.get<sf::Texture>(m_mapInfo[m_mapIndex].second));
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });

    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());

    thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::NextTrackPosition);
    thumbEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::TrackButtonSize };
    thumbEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_mapIndex = (m_mapIndex + 1) % m_mapInfo.size();
                    m_sharedData.mapName = m_mapInfo[m_mapIndex].first;

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::TrackThumb;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<xy::Sprite>().setTexture(m_resources.get<sf::Texture>(m_mapInfo[m_mapIndex].second));
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });

    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());

    //vehicle select
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition((xy::DefaultSceneSize.x - MenuConst::VehicleSelectArea.width) / 2.f, 180.f);
    entity.getComponent<xy::Transform>().move(0.f, -xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::VehicleSelect]));
    entity.addComponent<VehicleSelect>().index = m_sharedData.localPlayers[0].vehicle;
    entity.addComponent<xy::UIHitBox>().area = MenuConst::VehicleButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    e.getComponent<VehicleSelect>().index = (e.getComponent<VehicleSelect>().index + 1) % 3;
                    m_sharedData.localPlayers[0].vehicle = static_cast<std::uint32_t>(e.getComponent<VehicleSelect>().index);
                }
            });
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //lap select
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(114.f, -(xy::DefaultSceneSize.y - 238.f));
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 1);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::LapFrame]));
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto counterEnt = m_scene.createEntity();
    counterEnt.addComponent<xy::Transform>().setPosition(MenuConst::LapDigitPosition);
    counterEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    counterEnt.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::LapCounter]));
    counterEnt.addComponent<Digit>().value = m_sharedData.gameData.lapCount;
    counterEnt.addComponent<xy::CommandTarget>().ID = CommandID::Menu::LapCounter;
    entity.getComponent<xy::Transform>().addChild(counterEnt.getComponent<xy::Transform>());

    auto buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(MenuConst::LapPrevPosition);
    buttonEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::LapButtonSize };
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.gameData.lapCount = std::max(1, m_sharedData.gameData.lapCount - 1);

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::LapCounter;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<Digit>().value = m_sharedData.gameData.lapCount;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::Transform>().addChild(buttonEnt.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(MenuConst::LapNextPosition);
    buttonEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::LapButtonSize };
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.gameData.lapCount = std::min(99, m_sharedData.gameData.lapCount + 1);

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::LapCounter;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<Digit>().value = m_sharedData.gameData.lapCount;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::Transform>().addChild(buttonEnt.getComponent<xy::Transform>());
}

void MenuState::buildLocalPlayMenu(xy::Entity rootNode, sf::Uint32 mouseEnter, sf::Uint32 mouseExit)
{
    auto& uiSystem = m_scene.getSystem<xy::UISystem>();

    //back button
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuConst::NavLeftPosition);
    entity.getComponent<xy::Transform>().move(0.f, xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NavLeft];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
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
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //next button
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuConst::NavRightPosition);
    entity.getComponent<xy::Transform>().move(0.f, xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::NavRight];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    //launch mode based on current selection
                    requestStackClear();
                    if (m_eliminationMode)
                    {
                        requestStackPush(StateID::LocalElimination);
                    }
                    else
                    {
                        requestStackPush(StateID::LocalRace);
                    }
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //background
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::MenuBackground]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //map select
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(-140.f, xy::DefaultSceneSize.y * 1.26f);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::TrackSelect]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::ThumbnailPosition);
    thumbEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 1);
    thumbEnt.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::MonitorScreen));
    thumbEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    thumbEnt.getComponent<xy::Drawable>().setBlendMode(sf::BlendMultiply);
    thumbEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_mapInfo[m_mapIndex].second));
    thumbEnt.addComponent<xy::CommandTarget>().ID = CommandID::Menu::TrackThumb;
    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());

    thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::PrevTrackPosition);
    thumbEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::TrackButtonSize };
    thumbEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_mapIndex = (m_mapIndex + (m_mapInfo.size() - 1)) % m_mapInfo.size();
                    m_sharedData.mapName = m_mapInfo[m_mapIndex].first;

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::TrackThumb;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<xy::Sprite>().setTexture(m_resources.get<sf::Texture>(m_mapInfo[m_mapIndex].second));
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });

    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());

    thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::NextTrackPosition);
    thumbEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::TrackButtonSize };
    thumbEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_mapIndex = (m_mapIndex + 1) % m_mapInfo.size();
                    m_sharedData.mapName = m_mapInfo[m_mapIndex].first;

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::TrackThumb;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<xy::Sprite>().setTexture(m_resources.get<sf::Texture>(m_mapInfo[m_mapIndex].second));
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });

    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());

    //mode select
    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/mode_select.spt", m_resources);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(160.f, xy::DefaultSceneSize.y * 1.09f);
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("back");
    entity.addComponent<xy::SpriteAnimation>().play(0);
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::ModeSelectDialPosition);
    thumbEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 1);
    thumbEnt.addComponent<xy::Sprite>() = spriteSheet.getSprite("elimination");
    bounds = thumbEnt.getComponent<xy::Sprite>().getTextureBounds();
    thumbEnt.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    thumbEnt.addComponent<xy::Callback>().userData = std::make_any<float>(0.f);
    thumbEnt.getComponent<xy::Callback>().function =
        [](xy::Entity e, float dt)
    {
        const auto& target = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        auto& tx = e.getComponent<xy::Transform>();
        tx.rotate(xy::Util::Math::shortestRotation(tx.getRotation(), target)* dt * MenuConst::ModeSelectSpeed);

        float alpha = xy::Util::Math::clamp(1.f - (tx.getRotation() / 90.f), 0.f, 1.f);

        sf::Color c(255, 255, 255, static_cast<sf::Uint8>(alpha * 255.f));
        e.getComponent<xy::Sprite>().setColour(c);

        if (std::abs(target - tx.getRotation()) < 0.5f)
        {
            tx.setRotation(target);
            e.getComponent<xy::Callback>().active = false;
        }
    };
    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());
    auto elimEnt = thumbEnt;

    thumbEnt = m_scene.createEntity();
    thumbEnt.addComponent<xy::Transform>().setPosition(MenuConst::ModeSelectDialPosition);
    thumbEnt.getComponent<xy::Transform>().setRotation(-90.f);
    thumbEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 2);
    thumbEnt.addComponent<xy::Sprite>() = spriteSheet.getSprite("race");
    thumbEnt.getComponent<xy::Sprite>().setColour({ 255,255,255,0 });
    bounds = thumbEnt.getComponent<xy::Sprite>().getTextureBounds();
    thumbEnt.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    thumbEnt.addComponent<xy::Callback>().userData = std::make_any<float>(-90.f);
    thumbEnt.getComponent<xy::Callback>().function =
        [](xy::Entity e, float dt)
    {
        const auto& target = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        auto& tx = e.getComponent<xy::Transform>();
        tx.rotate(xy::Util::Math::shortestRotation(tx.getRotation(), target) * dt * MenuConst::ModeSelectSpeed);

        float alpha = xy::Util::Math::clamp(1.f - ((360.f - tx.getRotation()) / 90.f), 0.f, 1.f);

        sf::Color c(255, 255, 255, static_cast<sf::Uint8>(alpha * 255.f));
        e.getComponent<xy::Sprite>().setColour(c);

        if (std::abs(target - tx.getRotation()) < 0.5f)
        {
            tx.setRotation(target);
            e.getComponent<xy::Callback>().active = false;
        }
    };
    entity.getComponent<xy::Transform>().addChild(thumbEnt.getComponent<xy::Transform>());


    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);
    auto textEnt = m_scene.createEntity();
    textEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Sprite>().getTextureBounds().width / 2.f, 156.f);
    textEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 1);
    textEnt.addComponent<xy::Text>(font).setString("Elimination");
    textEnt.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    textEnt.getComponent<xy::Text>().setCharacterSize(24);
    entity.getComponent<xy::Transform>().addChild(textEnt.getComponent<xy::Transform>());

    entity.addComponent<xy::UIHitBox>().area = { sf::Vector2f(30.f, 47.f), {bounds.width, bounds.height} };
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&, elimEnt, thumbEnt, textEnt](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_eliminationMode = !m_eliminationMode;
                    if (m_eliminationMode)
                    {
                        std::any_cast<float&>(elimEnt.getComponent<xy::Callback>().userData) = 0.f;
                        std::any_cast<float&>(thumbEnt.getComponent<xy::Callback>().userData) = -90.f;
                        textEnt.getComponent<xy::Text>().setString("Elimination");
                    }
                    else
                    {
                        std::any_cast<float&>(elimEnt.getComponent<xy::Callback>().userData) = 90.f;
                        std::any_cast<float&>(thumbEnt.getComponent<xy::Callback>().userData) = 0.f;
                        textEnt.getComponent<xy::Text>().setString("Split Screen");
                    }
                    elimEnt.getComponent<xy::Callback>().active = true;
                    thumbEnt.getComponent<xy::Callback>().active = true;
                }
            });

    //lap select
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(176.f, (xy::DefaultSceneSize.y + 308.f));
    entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 1);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::LapFrame]));
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto counterEnt = m_scene.createEntity();
    counterEnt.addComponent<xy::Transform>().setPosition(MenuConst::LapDigitPosition);
    counterEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
    counterEnt.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::LapCounter]));
    counterEnt.addComponent<Digit>().value = m_sharedData.gameData.lapCount;
    counterEnt.addComponent<xy::CommandTarget>().ID = CommandID::Menu::LapCounter;
    entity.getComponent<xy::Transform>().addChild(counterEnt.getComponent<xy::Transform>());

    auto buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(MenuConst::LapPrevPosition);
    buttonEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::LapButtonSize };
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.gameData.lapCount = std::max(1, m_sharedData.gameData.lapCount - 1);

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::LapCounter;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<Digit>().value = m_sharedData.gameData.lapCount;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::Transform>().addChild(buttonEnt.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(MenuConst::LapNextPosition);
    buttonEnt.addComponent<xy::UIHitBox>().area = { sf::Vector2f(), MenuConst::LapButtonSize };
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.gameData.lapCount = std::min(99, m_sharedData.gameData.lapCount + 1);

                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::LapCounter;
                    cmd.action = [&](xy::Entity e, float)
                    {
                        e.getComponent<Digit>().value = m_sharedData.gameData.lapCount;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::Transform>().addChild(buttonEnt.getComponent<xy::Transform>());


    //vehicle select
    const std::array<sf::Vector2f, 4u> positions =
    {
        sf::Vector2f(443.f, 1110.f),
        sf::Vector2f(960.f, 1110.f),
        sf::Vector2f(443.f, 1394.f),
        sf::Vector2f(960.f, 1394.f)
    };

    for (auto i = 0; i < 4; ++i)
    {
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(positions[i]);
        entity.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
        entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::VehicleSelect]));
        entity.addComponent<VehicleSelect>().index = m_sharedData.localPlayers[i].vehicle;
        entity.addComponent<xy::UIHitBox>().area = MenuConst::VehicleButtonArea;
        entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            uiSystem.addMouseButtonCallback([&, i](xy::Entity e, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        e.getComponent<VehicleSelect>().index = (e.getComponent<VehicleSelect>().index + 1) % 3;
                        m_sharedData.localPlayers[i].vehicle = static_cast<std::uint32_t>(e.getComponent<VehicleSelect>().index);
                    }
                });
        rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        auto toggleEnt = m_scene.createEntity();
        toggleEnt.addComponent<xy::Transform>().setPosition(MenuConst::TogglePosition);
        toggleEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
        toggleEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::Menu::Toggle];

        if (m_sharedData.localPlayers[i].cpu)
        {
            auto bounds = toggleEnt.getComponent<xy::Sprite>().getTextureRect();
            bounds.top = 0.f;
            toggleEnt.getComponent<xy::Sprite>().setTextureRect(bounds);
        }
        toggleEnt.addComponent<xy::UIHitBox>().area = toggleEnt.getComponent<xy::Sprite>().getTextureBounds();
        toggleEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            uiSystem.addMouseButtonCallback([&, i](xy::Entity e, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        m_sharedData.localPlayers[i].cpu = !m_sharedData.localPlayers[i].cpu;
                        auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
                        bounds.top = (m_sharedData.localPlayers[i].cpu) ? 0.f : bounds.height;
                        e.getComponent<xy::Sprite>().setTextureRect(bounds);
                    }
                });
        entity.getComponent<xy::Transform>().addChild(toggleEnt.getComponent<xy::Transform>());

        textEnt = m_scene.createEntity();
        textEnt.addComponent<xy::Transform>().setPosition(64.f, 25.f);
        textEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth + 1);
        textEnt.addComponent<xy::Text>(font).setString("CPU");
        textEnt.getComponent<xy::Text>().setCharacterSize(24);
        toggleEnt.getComponent<xy::Transform>().addChild(textEnt.getComponent<xy::Transform>());

        auto lightEnt = m_scene.createEntity();
        lightEnt.addComponent<xy::Transform>().setPosition(MenuConst::LightbarPosition);
        lightEnt.addComponent<xy::Drawable>().setDepth(MenuConst::ButtonDepth);
        lightEnt.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Lightbar));
        lightEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        lightEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::LightBar]));
        lightEnt.getComponent<xy::Sprite>().setColour(GameConst::PlayerColour::Light[i]);
        entity.getComponent<xy::Transform>().addChild(lightEnt.getComponent<xy::Transform>());
    }
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