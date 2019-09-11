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
{
    m_defaultImage.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/default_player.png");
    
    for (auto i = 0u; i < m_clientInfo.size(); ++i)
    {
        auto& client = m_clientInfo[i];
        client.name = Global::PlayerNames[i];
        client.avatar.loadFromImage(m_defaultImage);
    }
}

//public
//bool ClientInfoManager::updateClient(const ConnectionState& state)
//{
//    XY_ASSERT(state.actorID >= Actor::ID::PlayerOne, "Invalid actor ID");
//
//    auto idx = state.actorID - Actor::ID::PlayerOne;
//    auto& client = m_clientInfo[idx];
//
//    {
//        if (state.clientID == 0)
//        {
//            //we're resetting a player
//            client.avatar.loadFromImage(m_defaultImage);
//            client.name = Global::PlayerNames[idx];
//            return true;
//        }
//        else //adding new info
//        {
//            //TODO pull new info from somewhere...
//        }
//    }
//    return false;
//}

//const ClientInfo& ClientInfoManager::getClient(const ConnectionState& state) const
//{
//    if (state.clientID > 0)
//    {
//        for (const auto& client : m_clientInfo)
//        {
//            if (client.clientID == state.clientID)
//            {
//                return client;
//            }
//        }
//    }
//    XY_ASSERT(state.actorID >= Actor::ID::PlayerOne, "Invalid actor ID");
//    return m_clientInfo[state.actorID - Actor::ID::PlayerOne];
//}

const ClientInfo& ClientInfoManager::getClient(std::int32_t idx) const
{
    XY_ASSERT(idx < m_clientInfo.size(), "Index is " + std::to_string(idx) + " and size is " + std::to_string(m_clientInfo.size()));
    return m_clientInfo[idx];
}

//private
//void ClientInfoManager::getSteamInfo(ClientInfo& client)
//{
//    XY_ASSERT(client.clientID > 0, "Not a valid client!");
//
//    client.name = "Fix Me";
//
//    auto avatarIdx = 0;
//
//    if (avatarIdx)
//    {
//        sf::Image img;
//        img.create(avatarSize, avatarSize, sf::Color::Black);
//        //TODO load some avatar image from somewhere
//        client.avatar.loadFromImage(img);
//    }
//    else
//    {
//        client.avatar.loadFromImage(m_defaultImage);
//    }
//}