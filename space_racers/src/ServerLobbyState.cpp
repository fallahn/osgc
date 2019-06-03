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
#include "Server.hpp"
#include "NetConsts.hpp"
#include "ClientPackets.hpp"
#include "ServerPackets.hpp"
#include "GameModes.hpp"

#include <xyginext/network/NetData.hpp>

#include <cstring>

using namespace sv;

namespace
{
    const sf::Time pingTime = sf::seconds(3.f);

    const std::uint8_t MinLaps = 1;
    const std::uint8_t MaxLaps = 9;
}

LobbyState::LobbyState(SharedData& sd, xy::MessageBus& mb)
    : m_sharedData  (sd),
    m_nextState     (StateID::Lobby),
    m_mapIndex      (0)
{
    for (auto& [id, player] : m_sharedData.playerInfo)
    {
        player.ready = false;
    }
    m_sharedData.lobbyData.playerCount = static_cast<std::uint8_t>(m_sharedData.clients.size());

    m_mapNames = xy::FileSystem::listFiles(xy::FileSystem::getResourcePath() + "assets/maps");
    m_mapNames.erase(std::remove_if(m_mapNames.begin(), m_mapNames.end(), 
        [](const std::string& str)
        {
            return xy::FileSystem::getFileExtension(str) != ".tmx";
        }), m_mapNames.end());
}

//public
void LobbyState::handleMessage(const xy::Message&)
{

};

void LobbyState::handleNetEvent(const xy::NetEvent& evt)
{
    if (evt.type == xy::NetEvent::PacketReceived)
    {
        const auto& packet = evt.packet;
        switch (packet.getID())
        {
        default: break;
        case PacketID::ReadyStateToggled:
            m_sharedData.playerInfo[evt.peer.getID()].ready = (evt.packet.as<std::uint8_t>() != 0);
            broadcastPlayerData(); //make sure clients are updated
            break;
        case PacketID::VehicleChanged:
            m_sharedData.playerInfo[evt.peer.getID()].vehicle = evt.packet.as<std::uint8_t>();
            broadcastPlayerData();
            break;
        case PacketID::NameString:
            setClientName(evt);
            break;
        case PacketID::LapCountChanged:
            if (packet.as<std::uint8_t>() == 0)
            {
                m_sharedData.lobbyData.lapCount
                    = (m_sharedData.lobbyData.lapCount == MinLaps) ? MinLaps : m_sharedData.lobbyData.lapCount - 2;
            }
            else
            {
                m_sharedData.lobbyData.lapCount = std::min(std::uint8_t(m_sharedData.lobbyData.lapCount + 2), MaxLaps);
            }
            m_sharedData.netHost.broadcastPacket(PacketID::LobbyData, m_sharedData.lobbyData, xy::NetFlag::Reliable);
            break;
        case PacketID::MapChanged:
            if (packet.as<std::uint8_t>() == 0)
            {
                m_mapIndex = (m_mapIndex + (m_mapNames.size() - 1)) % m_mapNames.size();
            }
            else
            {
                m_mapIndex = (m_mapIndex + 1) % m_mapNames.size();
            }
            broadcastMapName();
            m_sharedData.netHost.broadcastPacket(PacketID::LobbyData, m_sharedData.lobbyData, xy::NetFlag::Reliable);
            break;
        case PacketID::LaunchGame:
            for (const auto& [peer, player] : m_sharedData.playerInfo)
            {
                if (!player.ready)
                {
                    return;
                }
            }

            startGame();
            break;
        }
    }
    else if (evt.type == xy::NetEvent::ClientConnect)
    {
        if (m_sharedData.lobbyData.playerCount == LobbyData::MaxPlayers)
        {
            return;
        }

        //request info, like name - this is broadcast to other clients once it is received
        m_sharedData.netHost.sendPacket(evt.peer, PacketID::RequestPlayerName, std::uint8_t(0), xy::NetFlag::Reliable);

        m_sharedData.playerInfo[evt.peer.getID()].ready = false;
        m_sharedData.playerInfo[evt.peer.getID()].vehicle = 0;
        m_sharedData.lobbyData.playerCount++;
    }
    else if (evt.type == xy::NetEvent::ClientDisconnect)
    {
        //TODO hmmm we ought to transfer ownership if the host drops
        //but likelyhood is that the lobby was taken down anyway

        //clear client data, broadcast to other clients.
        auto result = std::find_if(m_sharedData.clients.begin(), m_sharedData.clients.end(),
            [&evt](const std::pair<xy::NetPeer, std::uint64_t>& pair)
            {
                return pair.first == evt.peer;
            });

        if (result != m_sharedData.clients.end())
        {
            auto id = result->second;
            m_sharedData.playerInfo.erase(id);
            m_sharedData.lobbyData.playerCount--;

            m_sharedData.netHost.broadcastPacket(PacketID::LeftLobby, id, xy::NetFlag::Reliable);
        }
    }
};

void LobbyState::netUpdate(float)
{

}

std::int32_t LobbyState::logicUpdate(float)
{
    if (m_broadcastClock.getElapsedTime() > pingTime)
    {
        broadcastNames();
        broadcastPlayerData();
        broadcastMapName();
        m_broadcastClock.restart();
    }

    return m_nextState; 
}

//private
void LobbyState::startGame()
{
    switch (m_sharedData.lobbyData.gameMode)
    {
    default: break;
    case GameMode::Race:
        m_nextState = StateID::Race;
        break;
    }

    //TODO ignore any further input in this state?
}

void LobbyState::setClientName(const xy::NetEvent& evt)
{
    auto size = std::min(evt.packet.getSize(), NetConst::MaxNameSize);
    std::vector<sf::Uint32> buffer(size / sizeof(sf::Uint32));
    std::memcpy(buffer.data(), evt.packet.getData(), size);

    m_sharedData.playerInfo[evt.peer.getID()].name = sf::String::fromUtf32(buffer.begin(), buffer.end());

    //broadcast update to all clients
    broadcastNames();
    broadcastPlayerData();
}

void LobbyState::broadcastNames() const
{
    for (const auto& [peerID, player] : m_sharedData.playerInfo)
    {
        auto size = std::min(NetConst::MaxNameSize, player.name.getSize() * sizeof(sf::Uint32));
        std::vector<char> buffer(sizeof(peerID) + size);

        std::memcpy(buffer.data(), &peerID, sizeof(peerID));
        std::memcpy(buffer.data() + sizeof(peerID), player.name.getData(), size);

        m_sharedData.netHost.broadcastPacket(PacketID::DeliverPlayerName, buffer.data(), size + sizeof(peerID), xy::NetFlag::Reliable);
    }
}

void LobbyState::broadcastPlayerData() const
{
    for (const auto& [peerID, player] : m_sharedData.playerInfo)
    {
        PlayerData data;
        data.peerID = peerID;
        data.ready = player.ready;
        data.vehicle = static_cast<std::uint8_t>(player.vehicle);

        m_sharedData.netHost.broadcastPacket(PacketID::DeliverPlayerData, data, xy::NetFlag::Reliable);
    }
    m_sharedData.netHost.broadcastPacket(PacketID::LobbyData, m_sharedData.lobbyData, xy::NetFlag::Reliable);
}

void LobbyState::broadcastMapName() const
{
    if (m_mapNames.empty())
    {
        m_sharedData.netHost.broadcastPacket(PacketID::ErrorServerGeneric, std::uint8_t(0), xy::NetFlag::Reliable);
    }
    else
    {
        auto size = std::min(NetConst::MaxNameSize, m_mapNames[m_mapIndex].size());
        std::vector<char> buffer(size);

        std::memcpy(buffer.data(), m_mapNames[m_mapIndex].data(), size);

        m_sharedData.netHost.broadcastPacket(PacketID::DeliverMapName, buffer.data(), size, xy::NetFlag::Reliable);
        m_sharedData.mapName = m_mapNames[m_mapIndex];
    }
}