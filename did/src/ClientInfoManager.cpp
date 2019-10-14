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

#include "ClientInfoManager.hpp"
#include "PacketTypes.hpp"
#include "GlobalConsts.hpp"

#include <SFML/Graphics/Image.hpp>

#include <xyginext/util/String.hpp>

ClientInfoManager::ClientInfoManager()
    : m_host(0)
{
    for (auto i = 0u; i < m_clientInfo.size(); ++i)
    {
        resetClient(i);
    }
}

//public
ClientInfo& ClientInfoManager::getClient(std::int32_t idx)
{
    XY_ASSERT(idx < m_clientInfo.size(), "Index is " + std::to_string(idx) + " and size is " + std::to_string(m_clientInfo.size()));
    return m_clientInfo[idx];
}

const ClientInfo& ClientInfoManager::getClient(std::int32_t idx) const
{
    XY_ASSERT(idx < m_clientInfo.size(), "Index is " + std::to_string(idx) + " and size is " + std::to_string(m_clientInfo.size()));
    return m_clientInfo[idx];
}

std::uint8_t ClientInfoManager::getClientFromPeer(std::uint64_t peer) const
{
    for (auto i = 0u; i < m_clientInfo.size(); ++i)
    {
        if (m_clientInfo[i].peerID == peer)
        {
            return i;
        }
    }
    return 255;
}

void ClientInfoManager::resetClient(std::int32_t idx)
{
    XY_ASSERT(idx < m_clientInfo.size(), "Index out of range");
    auto& client = m_clientInfo[idx];
    client.name = Global::PlayerNames[idx];
    client.peerID = 0;
    client.ready = true; //makes no actual odds, but looks like CPU players are ready
    client.gamesPlayed = 0;
    client.score = 0;
    client.spriteIndex = idx;
}

//private
