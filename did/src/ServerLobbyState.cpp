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
    xy::Logger::log("Server switched to lobby state");

    m_sharedData.connectedClients.erase(0);
    for (auto& client : m_sharedData.connectedClients)
    {
        client.second.ready = false; //reset the state for any currently connected players
    }
}

LobbyState::~LobbyState()
{

}

//public
void LobbyState::networkUpdate(float)
{

}

void LobbyState::logicUpdate(float)
{

}

void LobbyState::handlePacket(const xy::NetEvent& evt)
{
    switch (evt.packet.getID())
    {
    default: break;
    case PacketID::StartGame:
        //start game on request, check all players are ready first
    {
        bool ready = false;
        for (const auto& p : m_sharedData.connectedClients)
        {
            ready = p.second.ready;
            if (!ready)
            {
                break;
            }
        }

        if (ready)
        {
            setNextState(Server::StateID::Running);
            m_sharedData.gameServer->broadcastData(PacketID::LaunchGame, std::uint8_t(0), xy::NetFlag::Reliable);
        }
    }
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
        broadcastClientInfo();
        break;
    case PacketID::SupPlayerInfo:
    {
        PlayerInfoHeader ph;
        std::memcpy(&ph, (char*)evt.packet.getData(), sizeof(PlayerInfoHeader));
        m_sharedData.connectedClients[evt.peer.getID()].spriteIndex = ph.spriteIndex;
        m_sharedData.connectedClients[evt.peer.getID()].hatIndex = ph.hatIndex;

        //read out name string
        std::vector<sf::Uint32> buffer((evt.packet.getSize() - sizeof(ph)) / sizeof(sf::Uint32));
        std::memcpy(buffer.data(), (char*)evt.packet.getData() + sizeof(ph), evt.packet.getSize() - sizeof(ph));

        m_sharedData.connectedClients[evt.peer.getID()].name = sf::String::fromUtf32(buffer.begin(), buffer.end());
        m_sharedData.connectedClients[evt.peer.getID()].xp = 0;

        //update all clients
        broadcastClientInfo();
    }
        break;
    case PacketID::SetReadyState:
    {
        auto ready = evt.packet.as<std::uint8_t>();
        m_sharedData.connectedClients[evt.peer.getID()].ready = (ready != 0);

        std::uint16_t data = m_sharedData.connectedClients[evt.peer.getID()].playerID << 8;
        data |= ready;
        m_sharedData.gameServer->broadcastData(PacketID::GetReadyState, data, xy::NetFlag::Reliable);
    }
        break;
    case PacketID::ChatMessage:
        //we should have some sanity checks on this
        static auto const MaxSize = 322; //maxchar * sizeof(uint32) + 2 bytes
        if (evt.packet.getSize() < MaxSize //packet is not arbitrarily large...
            && evt.packet.getSize() > 2 //we actually have a string
            && ((const std::uint8_t*)evt.packet.getData())[1] >= sizeof(sf::Uint32))
        {
            m_sharedData.gameServer->broadcastData(PacketID::ChatMessage, evt.packet.getData(), evt.packet.getSize(), xy::NetFlag::Reliable);
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
        else if (data.type == ServerEvent::ClientDisconnected)
        {
            //broadcast player ident so clients can reset the slot
            m_sharedData.gameServer->broadcastData(PacketID::PlayerLeft, std::uint8_t(data.id), xy::NetFlag::Reliable);

            broadcastClientInfo();
        }
    }
}

//private
void LobbyState::broadcastClientInfo()
{
    for (const auto& [id, client] : m_sharedData.connectedClients)
    {
        //send the player number (0 - 3), peer ID, sprite index and name of client
        ClientInfoHeader ch;
        ch.playerID = client.playerID;
        ch.peerID = id;
        ch.spriteIndex = client.spriteIndex;
        ch.hatIndex = client.hatIndex;

        auto size = std::min(Global::MaxNameSize, client.name.getSize() * sizeof(sf::Uint32));
        std::vector<char> buffer(sizeof(ch) + size);

        std::memcpy(buffer.data(), &ch, sizeof(ch));
        std::memcpy(buffer.data() + sizeof(ch), client.name.getData(), size);

        m_sharedData.gameServer->broadcastData(PacketID::DeliverPlayerInfo, buffer.data(), buffer.size(), xy::NetFlag::Reliable);

        //client ready state
        std::uint16_t data = client.playerID << 8;
        if (client.ready)
        {
            data |= 1;
        }
        m_sharedData.gameServer->broadcastData(PacketID::GetReadyState, data, xy::NetFlag::Reliable);
    }

    //broadcast host peerid
    m_sharedData.gameServer->broadcastData(PacketID::HostID, m_sharedData.hostClient.getID(), xy::NetFlag::Reliable);
}