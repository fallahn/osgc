/*********************************************************************

Copyright 2019 Matt Marchant

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*********************************************************************/

#include "MenuState.hpp"
#include "GlobalConsts.hpp"
#include "Revision.hpp"
#include "MessageIDs.hpp"
#include "NetworkClient.hpp"
#include "SharedStateData.hpp"
#include "MixerChannels.hpp"
#include "PacketTypes.hpp"
#include "Packet.hpp"
#include "PluginExport.hpp"

#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>

#include <xyginext/core/ConfigFile.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/network/NetData.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>

namespace
{
    const sf::Vector2f MenuStart = { 40.f, 600.f };
    const float MenuSpacing = 80.f;
    const float PingTime = 1.f;
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    :xy::State      (ss, ctx),
    m_uiScene       (ctx.appInstance.getMessageBus()),
    m_gameScene     (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_pingServer    (false),
    m_gameLaunched  (false)
{
    launchLoadingScreen();
    loadAssets();
    createScene();

    setLobbyView();

    quitLoadingScreen();
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
    m_uiScene.getSystem<xy::UISystem>().handleEvent(evt);
    m_uiScene.forwardEvent(evt);
    m_gameScene.forwardEvent(evt);
    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    serverMessageHandler(m_sharedData, msg);

    if (msg.id == MessageID::SystemMessage)
    {
        const auto& data = msg.getData<SystemEvent>();
        if (data.action == SystemEvent::ServerStarted)
        {
            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::LobbyNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition({});
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = Menu::CommandID::MainNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            //TODO make sure this only happens if we aren't
            //already in a lobby - this message is also raised
            //when we inherit an existing one

            std::cout << "Join Lobby Here!\n";
        }
    }

    m_uiScene.forwardMessage(msg);
    m_gameScene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
#ifdef XY_DEBUG
    static std::size_t packetSize = 0;
    static std::size_t frames = 0;
    static float bw = 0.f;

    xy::NetEvent evt;
    while (m_sharedData.netClient->pollEvent(evt))
    {
        //TODO handle disconnect
        if (evt.type == xy::NetEvent::PacketReceived)
        {
            handlePacket(evt);
        }
        packetSize += evt.packet.getSize() + 1;
    }

    frames++;
    if (frames == 60)
    {
        //this is only an estimate...
        bw = static_cast<float>(packetSize * 8) / 1000.f;
        packetSize = 0;
        frames = 0;
    }
    xy::Console::printStat("Incoming bw", std::to_string(bw) + "Kbps");
#else
    xy::NetEvent evt;
    while (m_sharedData.netClient->pollEvent(evt))
    {
        //TODO handle disconnect
        if (evt.type == xy::NetEvent::PacketReceived)
        {
            handlePacket(evt);
        }
    }
#endif //XY_DEBUG

    m_uiScene.update(dt);
    m_gameScene.update(dt);

    if (m_pingServer && m_pingClock.getElapsedTime().asSeconds() > PingTime)
    {
        m_pingClock.restart();
        m_sharedData.netClient->sendPacket(PacketID::RequestSeed, std::uint8_t(0), xy::NetFlag::Reliable);
        std::cout << "Pinging...\n";
    }

    return true;
}

void MenuState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_gameScene);
    rw.draw(m_uiScene);
}

//private
void MenuState::loadAssets()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_uiScene.addSystem<xy::AudioSystem>(mb);
    m_uiScene.addSystem<xy::UISystem>(mb);
    m_uiScene.addSystem<xy::CommandSystem>(mb);
    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);

    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::TextSystem>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/lobby.spt", m_textureResource);
    m_sprites[Menu::SpriteID::Checkbox] = spriteSheet.getSprite("checkbox");
    m_sprites[Menu::SpriteID::Host] = spriteSheet.getSprite("host");
    m_sprites[Menu::SpriteID::BrowserItem] = xy::Sprite(m_textureResource.get("assets/images/browser_item.png"));

    m_callbackIDs[Menu::CallbackID::TextSelected] = m_uiScene.getSystem<xy::UISystem>().addSelectionCallback(
        [](xy::Entity entity)
    {
        entity.getComponent<xy::Text>().setFillColour(sf::Color::Red);
    });
    m_callbackIDs[Menu::CallbackID::TextUnselected] = m_uiScene.getSystem<xy::UISystem>().addSelectionCallback(
        [](xy::Entity entity)
    {
        entity.getComponent<xy::Text>().setFillColour(sf::Color::White);
    });

    m_callbackIDs[Menu::CallbackID::CheckboxClicked] = m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
        [&](xy::Entity entity, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            //check we're host and allowed to do this
            if (true) //TODO check we're the owner (friend only will probably go away anyway)
            {
                auto checked = entity.getComponent<std::uint8_t>() == 0 ? 1 : 0;
                entity.getComponent<std::uint8_t>() = checked;

                auto bounds = entity.getComponent<xy::Sprite>().getTextureRect();
                if (checked)
                {
                    bounds.top = bounds.height;
                    //TODO set friends only
                }
                else
                {
                    bounds.top = 0.f;
                    //TODO set public ... except of course there's no match making!
                }
                entity.getComponent<xy::Sprite>().setTextureRect(bounds);
            }
        }
    });

    m_callbackIDs[Menu::CallbackID::BrowserItemSelected] = m_uiScene.getSystem<xy::UISystem>().addSelectionCallback(
        [](xy::Entity entity)
    {
        auto bounds = entity.getComponent<xy::Sprite>().getTextureRect();
        bounds.top = bounds.height;
        entity.getComponent<xy::Sprite>().setTextureRect(bounds);
    });

    m_callbackIDs[Menu::CallbackID::BrowserItemDeselected] = m_uiScene.getSystem<xy::UISystem>().addSelectionCallback(
        [](xy::Entity entity)
    {
        auto bounds = entity.getComponent<xy::Sprite>().getTextureRect();
        bounds.top = 0.f;
        entity.getComponent<xy::Sprite>().setTextureRect(bounds);
    });

    m_callbackIDs[Menu::CallbackID::BrowserItemClicked] = m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
        [&](xy::Entity entity, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            //look for double click
            static const float DoubleClickTime = 0.3f;
            auto& clock = entity.getComponent<sf::Clock>();
            if (clock.getElapsedTime().asSeconds() < DoubleClickTime)
            {
                xy::Command cmd;
                cmd.targetFlags = Menu::CommandID::LobbyNode;
                cmd.action = [](xy::Entity e, float)
                {
                    e.getComponent<xy::Transform>().setPosition({});
                };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                cmd.targetFlags = Menu::CommandID::BrowserNode;
                cmd.action = [](xy::Entity e, float)
                {
                    e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
                };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                cmd.targetFlags = Menu::CommandID::BrowserItem;
                cmd.action = [&](xy::Entity entity, float)
                {
                    m_uiScene.destroyEntity(entity);
                };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
            else
            {
                clock.restart();
            }
        }
    });

    m_callbackIDs[Menu::CallbackID::ReadyBoxClicked] = m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
        [&](xy::Entity entity, sf::Uint64 flags)
    {
        if ((flags & xy::UISystem::Flags::LeftMouse)
            /*&& TODO check this entity has our peer ID so we know it's the local player*/)
        {
            auto selected = entity.getComponent<std::uint8_t>() == 0 ? 1 : 0;
            auto bounds = entity.getComponent<xy::Sprite>().getTextureRect();

            if (selected)
            {
                bounds.top = bounds.height;
                //TODO set player ready
            }
            else
            {
                bounds.top = 0.f;
                //TODO set player unready
            }
            entity.getComponent<std::uint8_t>() = selected;
            entity.getComponent<xy::Sprite>().setTextureRect(bounds);
        }
    });
}

void MenuState::createScene()
{
    //apply the default view
    auto view = getContext().defaultView;
    auto cameraEntity = m_uiScene.getActiveCamera();
    auto& camera = cameraEntity.getComponent<xy::Camera>();
    camera.setView(view.getSize());
    camera.setViewport(view.getViewport());

    auto& font = m_fontResource.get(Global::TitleFont);
    auto parentEntity = m_uiScene.createEntity();
    parentEntity.addComponent<xy::Transform>();
    parentEntity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::MainNode;
    parentEntity.addComponent<xy::AudioEmitter>().setSource("assets/sound/music/menu.ogg");
    parentEntity.getComponent<xy::AudioEmitter>().setChannel(MixerChannel::Music);
    parentEntity.getComponent<xy::AudioEmitter>().setLooped(true);
    parentEntity.getComponent<xy::AudioEmitter>().setVolume(0.15f);
    parentEntity.getComponent<xy::AudioEmitter>().play();

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(40.f, 40.f);
    entity.addComponent<xy::Text>(font).setString("Desert Island Duel");
    entity.getComponent<xy::Text>().setCharacterSize(Global::LargeTextSize);
    entity.addComponent<xy::Drawable>();
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    
    auto mouseOver = m_callbackIDs[Menu::CallbackID::TextSelected];
    auto mouseOut = m_callbackIDs[Menu::CallbackID::TextUnselected];


    //host
    //shows hosting window. Host may set options, and will create a local server
    //lobby server then set to this ID
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuStart);
    entity.addComponent<xy::Text>(font).setString("Host");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Selected] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Unselected] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            auto* msg = getContext().appInstance.getMessageBus().post<SystemEvent>(MessageID::SystemMessage);
            msg->action = SystemEvent::RequestStartServer;
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //join
    //shows lobby browser. Joining lobby shows browse window and joins
    //lobby with disabled option. inheriting lobby enables options
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuStart);
    entity.getComponent<xy::Transform>().move(0.f, MenuSpacing);
    entity.addComponent<xy::Text>(font).setString("Join");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Selected] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Unselected] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            parentEntity.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::BrowserNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition({});
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            refreshBrowserView();
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //options
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuStart);
    entity.getComponent<xy::Transform>().move(0.f, MenuSpacing * 2.f);
    entity.addComponent<xy::Text>(font).setString("Options");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Selected] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Unselected] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity ent, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            parentEntity.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::OptionsNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition({});
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //quit
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(MenuStart);
    entity.getComponent<xy::Transform>().move(0.f, MenuSpacing * 3.f);
    entity.addComponent<xy::Text>(font).setString("Quit");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Selected] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::Unselected] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity ent, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            requestStackClear();
            requestStackPush(StateID::ParentState);
        }
    });
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //version
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(20.f, 1020.f);
    entity.addComponent<xy::Text>(m_fontResource.get(Global::FineFont)).setString(REVISION_STRING);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize);
    entity.addComponent<xy::Drawable>();
    parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    buildLobby(font);
    buildBrowser(font);
    buildOptions(font);
}

void MenuState::setLobbyView()
{
    //check if we already have a lobby set, ie
    //if we were just in a game or were invited

    //in theory this should handle if a host quits the
    //game too and a new host was made, assuming the 
    //game mode properly quits

    //if (m_sharedData.lobbyID.IsLobby())
    //{
    //    xy::Command cmd;
    //    cmd.targetFlags = Menu::CommandID::LobbyNode;
    //    cmd.action = [](xy::Entity e, float)
    //    {
    //        e.getComponent<xy::Transform>().setPosition({});
    //    };
    //    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //    cmd.targetFlags = Menu::CommandID::MainNode;
    //    cmd.action = [](xy::Entity e, float)
    //    {
    //        e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    //    };
    //    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //    //allow players to join again if we're hosting
    //    if (m_sharedData.host == SteamUser()->GetSteamID())
    //    {
    //        SteamMatchmaking()->SetLobbyJoinable(m_sharedData.lobbyID, true);
    //    }

    //    refreshLobbyView();
    //    m_pingServer = true;
    //}
}

void MenuState::handlePacket(const xy::NetEvent& evt)
{
    switch (evt.packet.getID())
    {
    default: break;
    case PacketID::LaunchGame:
    {
        if (!m_gameLaunched) //only try launching on the first time we receive this
        {
            requestStackClear();
            requestStackPush(StateID::Game);

            m_gameLaunched = true;
        }
        break;
    }
    case PacketID::CurrentSeed:
    {
        m_sharedData.seedData = evt.packet.as<Server::SeedData>();

        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::SeedText;
        cmd.action = [&](xy::Entity entity, float)
        {
            entity.getComponent<xy::Text>().setString("Seed: " + std::string(m_sharedData.seedData.str));
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
        //we know we connected so we can stop pinging
        m_pingServer = false;

        break;
    }
}