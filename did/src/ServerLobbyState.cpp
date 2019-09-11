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

#include "ServerLobbyState.hpp"
#include "ServerSharedStateData.hpp"
#include "Server.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"
#include "MessageIDs.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/network/NetData.hpp>

#include <cstring>

using namespace Server;

namespace
{

}

LobbyState::LobbyState(SharedStateData& sd)
    : m_sharedData  (sd)
{
    setNextState(getID());
    xy::Logger::log("Server switched to active state");
}

LobbyState::~LobbyState()
{

}

//public
void LobbyState::networkUpdate(float dt)
{

}

void LobbyState::logicUpdate(float dt)
{

}

void LobbyState::handlePacket(const xy::NetEvent& evt)
{
    switch (evt.packet.getID())
    {
    default: break;
    case PacketID::StartGame:
        //start game on request TODO check all players are ready first
        LOG("Check all players are ready!", xy::Logger::Type::Warning);
        setNextState(Server::StateID::Running);
        m_sharedData.gameServer->broadcastData(PacketID::LaunchGame, std::uint8_t(0), xy::NetFlag::Reliable);
        break;
    case PacketID::SetSeed:
    {
        m_sharedData.seedData = evt.packet.as<Server::SeedData>();
        std::hash<std::string> hash;
        m_sharedData.seedData.hash = hash(std::string(m_sharedData.seedData.str));
    }
        m_sharedData.gameServer->broadcastData(PacketID::CurrentSeed, m_sharedData.seedData, xy::NetFlag::Reliable);
        break;
    case PacketID::RequestSeed:
        m_sharedData.gameServer->broadcastData(PacketID::CurrentSeed, m_sharedData.seedData, xy::NetFlag::Reliable);
        break;
    case PacketID::SupPlayerInfo:
    {
        //read out name string
        std::vector<sf::Uint32> buffer(evt.packet.getSize() / sizeof(sf::Uint32));
        std::memcpy(buffer.data(), (char*)evt.packet.getData(), evt.packet.getSize());

        m_sharedData.connectedClients[evt.peer.getID()].name = sf::String::fromUtf32(buffer.begin(), buffer.end());

        //update all clients
        broadcastClientInfo();
    }
        break;
    }
}

void LobbyState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::ServerMessage)
    {
        const auto& data = msg.getData<ServerEvent>();
        if (data.type == ServerEvent::ClientConnected)
        {
            //request info from client such as name and ready state
            m_sharedData.gameServer->sendData(PacketID::ReqPlayerInfo, std::uint8_t(0), data.id, xy::NetFlag::Reliable);
            m_sharedData.gameServer->sendData(PacketID::CurrentSeed, m_sharedData.seedData, data.id, xy::NetFlag::Reliable);
        }
    }
}

//private
void LobbyState::broadcastClientInfo()
{
    for (const auto& [id, client] : m_sharedData.connectedClients)
    {
        auto size = std::min(Global::MaxNameSize, client.name.getSize() * sizeof(sf::Uint32));
        std::vector<char> buffer(sizeof(id) + size);

        std::memcpy(buffer.data(), &id, sizeof(id));
        std::memcpy(buffer.data() + sizeof(id), client.name.getData(), size);

        m_sharedData.gameServer->broadcastData(PacketID::DeliverPlayerInfo, buffer.data(), std::size_t(size + sizeof(id)), xy::NetFlag::Reliable);
    }
}