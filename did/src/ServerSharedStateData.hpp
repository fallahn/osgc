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

#include <xyginext/network/NetData.hpp>

#include <SFML/System/String.hpp>

#include <map>

class GameServer;
namespace Server
{
    struct SeedData final
    {
        static constexpr std::size_t MaxChar = 12;
        char str[MaxChar] = {};
        std::size_t hash = 0;
    };

    struct ClientData final
    {
        xy::NetPeer peer;
        sf::String name = "Unknown";
        bool ready = false;
    };

    struct SharedStateData final
    {
        xy::NetPeer hostClient;
        GameServer* gameServer = nullptr;
        std::map<std::uint64_t, ClientData> connectedClients;
        SeedData seedData;
    };
}