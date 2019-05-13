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
        case PacketID::LobbyData:
            startGame(packet.as<LobbyData>());            
            break;
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
void LobbyState::startGame(const LobbyData& data)
{
    m_sharedData.playerCount = data.playerCount;
    m_sharedData.mapIndex = data.mapIndex;

    for (auto i = 0u; i < data.playerCount; ++i)
    {
        m_sharedData.peerIDs[i] = data.peerIDs[i];
        m_sharedData.vehicleIDs[i] = data.vehicleIDs[i];
    }

    switch (data.gameMode)
    {
    default: break;
    case GameMode::Race:
        m_nextState = StateID::Race;
        break;
    }

    //TODO ignore any further input in this state?
}