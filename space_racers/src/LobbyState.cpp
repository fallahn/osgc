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

#include "LobbyState.hpp"
#include "NetConsts.hpp"
#include "Server.hpp"
#include "ServerStates.hpp"
#include "ClientPackets.hpp"
#include "ServerPackets.hpp"
#include "GameModes.hpp"
#include "VehicleSystem.hpp"
#include "CommandIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

#include <cstring>

namespace
{
    const std::array<sf::String, 3u> vehicleNames =
    {
        "Car", "Bike", "ship"
    };

    const sf::Time pingTime = sf::seconds(5.f);
}

LobbyState::LobbyState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus())
{
    ctx.appInstance.setMouseCursorVisible(true);    
    
    launchLoadingScreen();
    initScene();
    loadResources();
    buildMenu();

    if (!sd.netClient) //first time joining
    {
        sd.netClient = std::make_unique<xy::NetClient>();
        if (!sd.netClient->create(2))
        {
            xy::Logger::log("Creating net client failed...", xy::Logger::Type::Error);
            sd.errorMessage = "Could not create network connection";
            requestStackPush(StateID::Error);
        }
        else
        {
            //launch a local server instance if hosting, then connect
            if (sd.hosting && !sd.server)
            {
                sd.server = std::make_unique<sv::Server>();
                sd.server->run(sv::StateID::Lobby);

                sf::Clock tempClock;
                while (tempClock.getElapsedTime().asSeconds() < 1.f) {} //give the server time to start
            }

            if (!sd.netClient->connect(sd.ip.toAnsiString(), NetConst::Port))
            {
                m_sharedData.errorMessage = "Failed to connect to lobby " + m_sharedData.ip;
                requestStackPush(StateID::Error);
            }
            else
            {
                auto id = sd.netClient->getPeer().getID();
                m_sharedData.playerInfo[id] = PlayerInfo();
                if (sd.name.isEmpty())
                {
                    m_sharedData.playerInfo[id].name = "Player " + std::to_string(id);
                }
                else
                {
                    m_sharedData.playerInfo[id].name = sd.name;
                }
            }
        }
    }
    quitLoadingScreen();
}

//public
bool LobbyState::handleEvent(const sf::Event& evt)
{
    m_scene.getSystem<xy::UISystem>().handleEvent(evt);
    m_scene.forwardEvent(evt);

    return true;
}

void LobbyState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool LobbyState::update(float dt)
{
    pollNetwork();

    if (m_pingClock.getElapsedTime() > pingTime)
    {
        m_sharedData.netClient->sendPacket(PacketID::ClientPing, std::int8_t(0), xy::NetFlag::Unreliable);
        m_pingClock.restart();
    }

    m_scene.update(dt);

    return true;
}

void LobbyState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void LobbyState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
    
}

void LobbyState::loadResources()
{
    if (FontID::handles[FontID::Default] == 0)
    {
        FontID::handles[FontID::Default] = m_sharedData.resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");
    }

    m_textureIDs[TextureID::Menu::VehicleSelect] = m_resources.load<sf::Texture>("assets/images/vehicle_select.png");

    xy::SpriteSheet spriteSheet;


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
}

void LobbyState::buildMenu()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);

    //title
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 40.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("LOBBY");
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(64);

    //body text
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(240.f, 220.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("No Players");
    entity.getComponent<xy::Text>().setCharacterSize(48);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Lobby::PlayerText;

    //lobby info
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(1440.f, 220.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Lobby");
    entity.getComponent<xy::Text>().setCharacterSize(48);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Lobby::LobbyText;

    //ready button
    auto& uiSystem = m_scene.getSystem<xy::UISystem>();
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(140.f, xy::DefaultSceneSize.y - 120.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Ready Up");
    entity.getComponent<xy::Text>().setCharacterSize(32);
    
    auto bounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    if (e.getComponent<xy::Text>().getFillColour() == sf::Color::White)
                    {
                        //not yet ready
                        e.getComponent<xy::Text>().setFillColour(sf::Color::Green);
                        m_sharedData.netClient->sendPacket(PacketID::ReadyStateToggled, std::uint8_t(1), xy::NetFlag::Reliable);
                    }
                    else
                    {
                        e.getComponent<xy::Text>().setFillColour(sf::Color::White);
                        m_sharedData.netClient->sendPacket(PacketID::ReadyStateToggled, std::uint8_t(0), xy::NetFlag::Reliable);
                    }
                }
            });

    //quit button
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - 240.f, xy::DefaultSceneSize.y - 120.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Leave");
    entity.getComponent<xy::Text>().setCharacterSize(32);

    bounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    if (m_sharedData.server)
                    {
                        e.getComponent<xy::Text>().setString("Quitting...");
                        m_sharedData.server->quit();
                    }

                    m_sharedData.playerInfo.clear();

                    requestStackClear();
                    requestStackPush(StateID::MainMenu);
                }
            });

    //vehicle select buttons
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(400.f, xy::DefaultSceneSize.y - 136.f);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::VehicleSelect]));
    entity.addComponent<xy::Drawable>();
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    bounds.height /= 3.f;
    entity.getComponent<xy::Sprite>().setTextureRect(bounds);

    bounds.width /= 3.f;
    auto buttonEnt = m_scene.createEntity();
    entity.getComponent<xy::Transform>().addChild(buttonEnt.addComponent<xy::Transform>());
    buttonEnt.addComponent<xy::UIHitBox>().area = bounds;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&,entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    auto id = m_sharedData.netClient->getPeer().getID();
                    m_sharedData.playerInfo[id].vehicle = (m_sharedData.playerInfo[id].vehicle + 2) % 3;

                    auto rect = entity.getComponent<xy::Sprite>().getTextureBounds();
                    rect.top = m_sharedData.playerInfo[id].vehicle * rect.height;
                    entity.getComponent<xy::Sprite>().setTextureRect(rect);

                    m_sharedData.netClient->sendPacket(PacketID::VehicleChanged, std::uint8_t(m_sharedData.playerInfo[id].vehicle), xy::NetFlag::Reliable);
                }
            });

    buttonEnt = m_scene.createEntity();
    entity.getComponent<xy::Transform>().addChild(buttonEnt.addComponent<xy::Transform>());
    buttonEnt.getComponent<xy::Transform>().move(bounds.width* 2.f, 0.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = bounds;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    auto id = m_sharedData.netClient->getPeer().getID();
                    m_sharedData.playerInfo[id].vehicle = (m_sharedData.playerInfo[id].vehicle + 1) % 3;

                    auto rect = entity.getComponent<xy::Sprite>().getTextureBounds();
                    rect.top = m_sharedData.playerInfo[id].vehicle * rect.height;
                    entity.getComponent<xy::Sprite>().setTextureRect(rect);

                    m_sharedData.netClient->sendPacket(PacketID::VehicleChanged, std::uint8_t(m_sharedData.playerInfo[id].vehicle), xy::NetFlag::Reliable);
                }
            });

    if (m_sharedData.hosting)
    {
        //TODO map selection

        //lap count buttons
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - 400.f, xy::DefaultSceneSize.y - 136.f);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::VehicleSelect]));
        entity.addComponent<xy::Drawable>();
        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        bounds.height /= 3.f;
        entity.getComponent<xy::Sprite>().setTextureRect(bounds);
        entity.getComponent<xy::Transform>().move(-bounds.width, 0.f);

        bounds.width /= 3.f;
        auto buttonEnt = m_scene.createEntity();
        entity.getComponent<xy::Transform>().addChild(buttonEnt.addComponent<xy::Transform>());
        buttonEnt.addComponent<xy::UIHitBox>().area = bounds;
        buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            uiSystem.addMouseButtonCallback([&, entity](xy::Entity, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        m_sharedData.netClient->sendPacket(PacketID::LapCountChanged, std::uint8_t(0), xy::NetFlag::Reliable);
                    }
                });

        buttonEnt = m_scene.createEntity();
        entity.getComponent<xy::Transform>().addChild(buttonEnt.addComponent<xy::Transform>());
        buttonEnt.getComponent<xy::Transform>().move(bounds.width * 2.f, 0.f);
        buttonEnt.addComponent<xy::UIHitBox>().area = bounds;
        buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            uiSystem.addMouseButtonCallback([&, entity](xy::Entity, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        m_sharedData.netClient->sendPacket(PacketID::LapCountChanged, std::uint8_t(1), xy::NetFlag::Reliable);
                    }
                });

        //start game button
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, xy::DefaultSceneSize.y - 120.f);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font).setString("START");
        entity.getComponent<xy::Text>().setCharacterSize(64);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);

        bounds = xy::Text::getLocalBounds(entity);
        entity.addComponent<xy::UIHitBox>().area = bounds;
        entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
                {
                    if (flags & xy::UISystem::LeftMouse)
                    {
                        m_sharedData.netClient->sendPacket(PacketID::LaunchGame, std::uint8_t(0), xy::NetFlag::Reliable);
                    }
                });
    }
}

void LobbyState::pollNetwork()
{
    xy::NetEvent evt;
    while (m_sharedData.netClient->pollEvent(evt))
    {
        if (evt.type == xy::NetEvent::PacketReceived)
        {
            const auto& packet = evt.packet;
            switch (packet.getID())
            {
            default: break;
            case PacketID::LobbyData:
                updateLobbyData(packet.as<LobbyData>());
                break;
            case PacketID::ErrorServerDisconnect:
                m_sharedData.errorMessage = "Host Disconnected.";
                m_sharedData.netClient->disconnect();
                requestStackPush(StateID::Error);
                break;
            case PacketID::ErrorServerFull:
                m_sharedData.errorMessage = "Could not connect, server full";
                m_sharedData.netClient->disconnect();
                requestStackPush(StateID::Error);
                break;
            case PacketID::LeftLobby:
                m_sharedData.playerInfo.erase(packet.as<std::uint64_t>());
                refreshView();
                break;
            case PacketID::RequestPlayerName:
                sendPlayerName();
                break;
            case PacketID::DeliverPlayerData:
            {
                auto data = packet.as<PlayerData>();
                m_sharedData.playerInfo[data.peerID].ready = data.ready;
                m_sharedData.playerInfo[data.peerID].vehicle = data.vehicle;
                refreshView();
            }
                break;
            case PacketID::DeliverPlayerName:
                receivePlayerName(evt);
                refreshView();
                break;
            case PacketID::GameStarted:
                //set shared data with game info
                //and launch game state
            {
                auto data = packet.as<GameStart>();
                m_sharedData.gameData.actorCount = data.actorCount;
                m_sharedData.gameData.mapIndex = data.mapIndex;

                switch (data.gameMode)
                {
                default: break; //only network races implemented...
                case GameMode::Race:
                    requestStackClear();
                    requestStackPush(StateID::Race);
                    break;
                }
            }
            break;
            case PacketID::ErrorServerMap:
                LOG("Server failed to load map - TODO report map index which failed", xy::Logger::Type::Error);
                m_sharedData.errorMessage = "Server failed to load map";
                requestStackPush(StateID::Error);
                break;
            }
        }
    }
}

void LobbyState::sendPlayerName()
{
    auto nameBytes = m_sharedData.playerInfo[m_sharedData.netClient->getPeer().getID()].name.toUtf32();
    auto size = std::min(nameBytes.size() * sizeof(sf::Uint32), NetConst::MaxNameSize);

    m_sharedData.netClient->sendPacket(PacketID::NameString, nameBytes.data(), size, xy::NetFlag::Reliable);
}

void LobbyState::receivePlayerName(const xy::NetEvent& evt)
{
    auto size = evt.packet.getSize();
    size = std::min(NetConst::MaxNameSize, size - sizeof(sf::Uint64));
    
    sf::Uint64 peerID = 0;
    std::memcpy(&peerID, evt.packet.getData(), sizeof(peerID));

    std::vector<sf::Uint32> buffer(size / sizeof(sf::Uint32));
    std::memcpy(buffer.data(), (char*)evt.packet.getData() + sizeof(peerID), size);

    m_sharedData.playerInfo[peerID].name = sf::String::fromUtf32(buffer.begin(), buffer.end());
}

void LobbyState::refreshView()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::Lobby::PlayerText;
    cmd.action = [&](xy::Entity e, float)
    {
        static const auto ColumnWidth = (NetConst::MaxNameSize / sizeof(std::uint32_t)) + 6;
        
        sf::String output;

        for (const auto& [peer, player] : m_sharedData.playerInfo)
        {
            output += player.name;
            for (auto i = 0u; i < (ColumnWidth - player.name.getSize()); ++i)
            {
                output += " ";
            }

            output += "Score: " + std::to_string(player.score) + "\n";
            output += vehicleNames[player.vehicle];
            if (player.ready)
            {
                output += " - Ready\n\n";
            }
            else
            {
                output += " - Not Ready\n\n";
            }
        }
        e.getComponent<xy::Text>().setString(output);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void LobbyState::updateLobbyData(const LobbyData& data)
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::Lobby::LobbyText;
    cmd.action = [data](xy::Entity e, float)
    {
        std::string str("Lobby Info\n");
        str += "Map :" + std::to_string(data.mapIndex) + "\n"; //TODO look up map names
        str += "Laps: " + std::to_string(data.lapCount) + "\n";
        str += "Mode: " + std::to_string(data.gameMode) + "\n"; //TODO look up mode names
        e.getComponent<xy::Text>().setString(str);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}