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

        //TODO request info, like name
        m_sharedData.lobbyData.peerIDs[m_sharedData.lobbyData.playerCount] = evt.peer.getID();
        m_sharedData.lobbyData.vehicleIDs[m_sharedData.lobbyData.playerCount] = 0;
        m_sharedData.lobbyData.playerCount++;
    }
    else if (evt.type == xy::NetEvent::ClientDisconnect)
    {
        //TODO hmmm we ought to transfer ownership if the host drops
        //but likelyhood is that the lobby was taken down anyway

        //clear client data, broadcast to other clients.
        auto id = evt.peer.getID();
        auto i = 0u;

        //find who left
        for (i; i < m_sharedData.lobbyData.playerCount; ++i)
        {
            if (m_sharedData.lobbyData.peerIDs[i] == id)
            {
                break;
            }
        }

        //swap last player into empty slot
        if (m_sharedData.lobbyData.playerCount > 1)
        {
            m_sharedData.lobbyData.playerCount--;
            if (i != m_sharedData.lobbyData.playerCount)
            {
                m_sharedData.lobbyData.peerIDs[i] = m_sharedData.lobbyData.peerIDs[m_sharedData.lobbyData.playerCount];
                m_sharedData.lobbyData.vehicleIDs[i] = m_sharedData.lobbyData.vehicleIDs[m_sharedData.lobbyData.playerCount];
            }
        }
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