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
#include "ResourceIDs.hpp"
#include "CommandIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

#include <cstring>

namespace
{
    const std::array<sf::String, 3u> vehicleNames =
    {
        "Car", "Bike", "ship"
    };
}

LobbyState::LobbyState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();
    initScene();

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

        if (!sd.netClient->connect(sd.ip, NetConst::Port))
        {
            m_sharedData.errorMessage = "Failed to connect to lobby " + m_sharedData.ip;
            requestStackPush(StateID::Error);
        }
        else
        {
            auto id = sd.netClient->getPeer().getID();
            m_playerInfo[id] = PlayerInfo();
            m_playerInfo[id].name = "Player " + std::to_string(id);
        }
    }

    quitLoadingScreen();
}

//public
bool LobbyState::handleEvent(const sf::Event& evt)
{
    //temp just to launch a game
    if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == sf::Keyboard::Space
            && m_sharedData.hosting)
        {
            m_sharedData.netClient->sendPacket(PacketID::LaunchGame, std::uint8_t(0), xy::NetFlag::Reliable);

            //TODO we need to ignore further input from other clients to the lobby
            //perhaps relay this from the server when it rx lobby data
        }
    }

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
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
    
    FontID::handles[FontID::Default] = m_sharedData.resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 40.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("LOBBY");
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(32);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(40.f, 120.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("No Players");
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::LobbyText;
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
            case PacketID::LeftLobby:
                m_playerInfo.erase(packet.as<std::uint64_t>());
                refreshView();
                break;
            case PacketID::RequestPlayerName:
                sendPlayerName();
                break;
            case PacketID::DeliverPlayerData:
            {
                auto data = packet.as<PlayerData>();
                m_playerInfo[data.peerID].ready = data.ready;
                m_playerInfo[data.peerID].vehicle = data.vehicle;
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
    auto nameBytes = m_playerInfo[m_sharedData.netClient->getPeer().getID()].name.toUtf32();
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

    m_playerInfo[peerID].name = sf::String::fromUtf32(buffer.begin(), buffer.end());
}

void LobbyState::refreshView()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::LobbyText;
    cmd.action = [&](xy::Entity e, float)
    {
        sf::String output;

        for (const auto& [peer, player] : m_playerInfo)
        {
            output += player.name + " - " + vehicleNames[player.vehicle];
            if (player.ready)
            {
                output += " - Ready";
            }
            else
            {
                output += " - Not Ready\n";
            }
        }
        e.getComponent<xy::Text>().setString(output);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}