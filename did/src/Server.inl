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

#pragma once

#include <xyginext/core/Log.hpp>

template <typename T>
inline void GameServer::sendData(std::uint8_t packetID, const T& data, std::uint64_t destination, xy::NetFlag sendType, std::uint8_t channel)
{
#ifdef XY_DEBUG
    if (m_sharedStateData.connectedClients.count(destination) == 0)
    {
        LogW << "Tried to send to " << destination << ": invalid destination" << std::endl;
        return;
    }
#endif

    m_host.sendPacket(m_sharedStateData.connectedClients[destination].peer, packetID, data, sendType, channel);
}

template <typename T>
inline void GameServer::broadcastData(std::uint8_t packetID, const T& data, xy::NetFlag sendType, std::uint8_t channel)
{
    m_host.broadcastPacket(packetID, data, sendType, channel);
}