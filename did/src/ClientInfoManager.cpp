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

namespace
{
    const sf::Uint32 avatarSize = 64u;
}

ClientInfoManager::ClientInfoManager()
    : m_host(0)
{
    m_defaultImage.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/default_player.png");
    
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

void ClientInfoManager::resetClient(std::int32_t idx)
{
    auto& client = m_clientInfo[idx];
    client.name = Global::PlayerNames[idx];
    client.avatar.loadFromImage(m_defaultImage);
    client.peerID = 0;
    client.ready = true; //makes no actual odds, but looks like CPU players are ready
}

//private
