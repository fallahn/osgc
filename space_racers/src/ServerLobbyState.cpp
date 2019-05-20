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
#include "GameModes.hpp"

#include <xyginext/network/NetData.hpp>

#include <cstring>

using namespace sv;
LobbyState::LobbyState(SharedData& sd, xy::MessageBus& mb)
    : m_sharedData  (sd),
    m_nextState     (StateID::Lobby)
{

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
        case PacketID::NameString:
            setClientName(evt);
            break;
        case PacketID::LaunchGame:
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
        m_sharedData.netHost.sendPacket(evt.peer, PacketID::RequestPlayerData, std::uint8_t(0), xy::NetFlag::Reliable);

        m_sharedData.playerInfo[evt.peer.getID()].ready = false;
        m_sharedData.playerInfo[evt.peer.getID()].vehicle = 0;
        m_sharedData.lobbyData.playerCount++;
    }
    else if (evt.type == xy::NetEvent::ClientDisconnect)
    {
        //TODO hmmm we ought to transfer ownership if the host drops
        //but likelyhood is that the lobby was taken down anyway

        //clear client data, broadcast to other clients.
        auto id = evt.peer.getID();
        m_sharedData.playerInfo.erase(id);
        m_sharedData.lobbyData.playerCount--;

        m_sharedData.netHost.broadcastPacket(PacketID::LeftLobby, id, xy::NetFlag::Reliable);
    }
};

void LobbyState::netUpdate(float)
{

}

std::int32_t LobbyState::logicUpdate(float)
{
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

    //TODO broadcast update to all clients
}