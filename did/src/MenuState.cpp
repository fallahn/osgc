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
#include <xyginext/gui/Gui.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Window/Event.hpp>

#include <fstream>

namespace
{
    const sf::Vector2f MenuStart = { 40.f, 600.f };
    const float MenuSpacing = 80.f;
    const float PingTime = 1.f;

    const std::string NameFile("name.set");
    const std::string AddressFile("address.set");
    const std::vector<std::string> names =
    {
        "Cleftwisp", "Old Sam", "Lou Baker",
        "Geoff Strongman", "Issy Soux", "Grumbles",
        "Harriet Cobbler", "Queasy Jo", "E. Claire",
        "Pro Moist", "Hannah Theresa", "Shrinkwrap"
    };
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    :xy::State      (ss, ctx),
    m_uiScene       (ctx.appInstance.getMessageBus()),
    m_gameScene     (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_pingServer    (false),
    m_gameLaunched  (false),
    m_activeString  (nullptr)
{
    launchLoadingScreen();

    loadSettings();
    loadAssets();
    createScene();

    setLobbyView();

    quitLoadingScreen();
}

MenuState::~MenuState()
{
    saveSettings();
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
    if (xy::ui::wantsMouse() || xy::ui::wantsKeyboard())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
#ifdef XY_DEBUG
        case sf::Keyboard::Escape:
            xy::App::quit();
            break;
#endif
        }
    }

    updateTextInput(evt);

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

            cmd.targetFlags = Menu::CommandID::HostNameNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            if (!m_sharedData.netClient->connected())
            {
                m_sharedData.netClient->connect("127.0.0.1", Global::GamePort);
                //TODO if failure then return to main menu
            }
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

    //TODO do we need this now?
    if (m_pingServer && m_pingClock.getElapsedTime().asSeconds() > PingTime)
    {
        m_pingClock.restart();
        m_sharedData.netClient->sendPacket(PacketID::RequestSeed, std::uint8_t(0), xy::NetFlag::Reliable, Global::ReliableChannel);
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
void MenuState::updateTextInput(const sf::Event& evt)
{
    if (m_activeString != nullptr)
    {
        std::size_t maxChar = (m_activeString == &m_sharedData.remoteIP)
            ? 15 : Global::MaxNameSize / sizeof(sf::Uint32);

        std::uint32_t targetFlags = (m_activeString != &m_sharedData.clientName)
            ? Menu::CommandID::IPText : Menu::CommandID::NameText;

        auto updateText = [&, targetFlags]()
        {
            xy::Command cmd;
            cmd.targetFlags = targetFlags;
            cmd.action = [&](xy::Entity entity, float)
            {
                auto& text = entity.getComponent<xy::Text>();
                text.setString(*m_activeString);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
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
                cmd.targetFlags = Menu::CommandID::NameText | Menu::CommandID::IPText;
                cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
        }
    }
}

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

    m_callbackIDs[Menu::CallbackID::TextSelected] = m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback(
        [](xy::Entity entity, sf::Vector2f)
    {
        entity.getComponent<xy::Text>().setFillColour(sf::Color::Red);
    });
    m_callbackIDs[Menu::CallbackID::TextUnselected] = m_uiScene.getSystem<xy::UISystem>().addMouseMoveCallback(
        [](xy::Entity entity, sf::Vector2f)
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
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::MainNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = Menu::CommandID::HostNameNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition({});
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            m_activeString = &m_sharedData.clientName;

            cmd.targetFlags = Menu::CommandID::NameText;
            cmd.action = [&](xy::Entity e, float)
            {
                e.getComponent<xy::Text>().setFillColour(sf::Color::Red);
                e.getComponent<xy::Text>().setString(m_sharedData.clientName);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
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
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEntity](xy::Entity, sf::Uint64 flags) mutable
    {
        if (flags & xy::UISystem::Flags::LeftMouse)
        {
            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::MainNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = Menu::CommandID::JoinNameNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Transform>().setPosition({});
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            m_activeString = &m_sharedData.clientName;

            cmd.targetFlags = Menu::CommandID::NameText;
            cmd.action = [&](xy::Entity e, float)
            {
                e.getComponent<xy::Text>().setFillColour(sf::Color::Red);
                e.getComponent<xy::Text>().setString(m_sharedData.clientName);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = Menu::CommandID::IPText;
            cmd.action = [&](xy::Entity e, float)
            {
                e.getComponent<xy::Text>().setFillColour(sf::Color::White);
                e.getComponent<xy::Text>().setString(m_sharedData.remoteIP);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
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
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
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
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = mouseOver;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = mouseOut;
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
    //buildBrowser(font);
    buildOptions(font);

    buildNameEntry(font);
    buildJoinEntry(font);
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
    case PacketID::ReqPlayerInfo:
        sendPlayerData();
        break;
    case PacketID::DeliverPlayerInfo:
        refreshLobbyView(evt);
        break;
    }
}

void MenuState::buildNameEntry(sf::Font& largeFont)
{
    auto parentEnt = m_uiScene.createEntity();
    parentEnt.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    parentEnt.addComponent<xy::CommandTarget>().ID = Menu::CommandID::HostNameNode;

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/text_input.png"));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags) 
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_sharedData.clientName;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto& font = m_fontResource.get(Global::FineFont);

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 2.5f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Text>(font).setString("Enter Your Name");
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 0.5f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.getComponent<xy::Drawable>().setCroppingArea({ -232.f, -30.f, 464.f, 120.f });
    entity.addComponent<xy::Text>(font).setString(m_sharedData.clientName);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 14);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::NameText;
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    //back button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Text>(largeFont).setString("Back");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Left);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEnt](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    parentEnt.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::MainNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<xy::Transform>().setPosition({});
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    m_activeString = nullptr;

                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    if (m_sharedData.clientName.isEmpty())
                    {
                        m_sharedData.clientName = names[xy::Util::Random::value(0, names.size() - 1)];
                    }
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //next button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Text>(largeFont).setString("Next");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().area.left = -entity.getComponent<xy::UIHitBox>().area.width;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    if (!m_sharedData.clientName.isEmpty())
                    {
                        auto* msg = getContext().appInstance.getMessageBus().post<SystemEvent>(MessageID::SystemMessage);
                        msg->action = SystemEvent::RequestStartServer;

                        m_activeString = nullptr;

                        xy::Command cmd;
                        cmd.targetFlags = Menu::CommandID::NameText;
                        cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                    }
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void MenuState::buildJoinEntry(sf::Font& largeFont)
{
    auto parentEnt = m_uiScene.createEntity();
    parentEnt.addComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
    parentEnt.addComponent<xy::CommandTarget>().ID = Menu::CommandID::JoinNameNode;

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/text_input.png"));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_sharedData.clientName;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = Menu::CommandID::IPText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Far);
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/text_input.png"));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, bounds.height * 1.4f);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseDown] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeString = &m_sharedData.remoteIP;

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::IPText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::Red); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = Menu::CommandID::NameText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    auto& font = m_fontResource.get(Global::FineFont);

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 2.5f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.addComponent<xy::Text>(font).setString("Enter Your Name");
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 0.5f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.getComponent<xy::Drawable>().setCroppingArea({ -232.f, -30.f, 464.f, 120.f });
    entity.addComponent<xy::Text>(font).setString(m_sharedData.clientName);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 14);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::NameText;
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, bounds.height * 0.9f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
    entity.getComponent<xy::Drawable>().setCroppingArea({ -232.f, -30.f, 464.f, 120.f });
    entity.addComponent<xy::Text>(font).setString(m_sharedData.remoteIP);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 14);
    entity.addComponent<xy::CommandTarget>().ID = Menu::CommandID::IPText;
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    //back button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::BackButtonPosition);
    entity.addComponent<xy::Text>(largeFont).setString("Back");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Left);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&, parentEnt](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    parentEnt.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);

                    xy::Command cmd;
                    cmd.targetFlags = Menu::CommandID::MainNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<xy::Transform>().setPosition({});
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    m_activeString = nullptr;

                    cmd.targetFlags = Menu::CommandID::NameText | Menu::CommandID::IPText;
                    cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    if (m_sharedData.clientName.isEmpty())
                    {
                        m_sharedData.clientName = names[xy::Util::Random::value(0, names.size() - 1)];
                    }

                    if (m_sharedData.remoteIP.isEmpty())
                    {
                        m_sharedData.remoteIP = "127.0.0.1";
                    }
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //next button
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::StartButtonPosition);
    entity.addComponent<xy::Text>(largeFont).setString("Join");
    entity.getComponent<xy::Text>().setCharacterSize(Global::MediumTextSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Right);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseEnter] = m_callbackIDs[Menu::CallbackID::TextSelected];
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseExit] = m_callbackIDs[Menu::CallbackID::TextUnselected];
    entity.getComponent<xy::UIHitBox>().area = Menu::ButtonArea;
    entity.getComponent<xy::UIHitBox>().area.left = -entity.getComponent<xy::UIHitBox>().area.width;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        m_uiScene.getSystem<xy::UISystem>().addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::Flags::LeftMouse)
                {
                    if (!m_sharedData.clientName.isEmpty()
                        && !m_sharedData.remoteIP.isEmpty())
                    {
                        m_activeString = nullptr;

                        xy::Command cmd;
                        cmd.targetFlags = Menu::CommandID::NameText | Menu::CommandID::IPText;
                        cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
                        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                        //TODO actually join game
                    }
                }
            });
    parentEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

}

void MenuState::loadSettings()
{
    auto settingsPath = xy::FileSystem::getConfigDirectory(getContext().appInstance.getApplicationName());
    
    //if settings not found set some default values
    if (!xy::FileSystem::fileExists(settingsPath + NameFile))
    {
        m_sharedData.clientName = names[xy::Util::Random::value(0, names.size() - 1)];
    }
    else
    {
        //TODO wrap in a function so can nicely repeat for IP address
        std::ifstream file(settingsPath + NameFile, std::ios::binary);
        if (file.is_open() && file.good())
        {
            file.seekg(0, file.end);
            auto fileSize = file.tellg();
            file.seekg(file.beg);

            std::vector<char> buffer(fileSize);
            file.read(buffer.data(), fileSize);
            file.close();

            m_sharedData.clientName = sf::String::fromUtf32(buffer.begin(), buffer.end());
        }
    }

    if (!xy::FileSystem::fileExists(settingsPath + AddressFile))
    {
        m_sharedData.remoteIP = "255.255.255.255";
    }
    else
    {

    }
}

void MenuState::saveSettings()
{
    auto settingsPath = xy::FileSystem::getConfigDirectory(getContext().appInstance.getApplicationName());

    std::ofstream file(settingsPath + NameFile, std::ios::binary);
    if (file.is_open() && file.good())
    {
        auto codePoints = m_sharedData.clientName.toUtf32();
        //file.write(reinterpret_cast<char*>(codePoints.data()), codePoints.size() * sizeof(sf::Uint32));
        file.close();
    }

    file.open(settingsPath + AddressFile, std::ios::binary);
    if (file.is_open() && file.good())
    {
        auto codePoints = m_sharedData.remoteIP.toUtf32();
        //file.write(reinterpret_cast<char*>(codePoints.data()), codePoints.size() * sizeof(sf::Uint32));
        file.close();
    }
}