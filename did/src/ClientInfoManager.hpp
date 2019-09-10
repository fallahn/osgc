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

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/System/String.hpp>

#include <array>

struct ConnectionState;
struct ClientInfo final
{
    sf::String name;
    sf::Texture avatar;
    std::uint64_t clientID = 0;
};

class ClientInfoManager final
{
public:
    ClientInfoManager();

    bool updateClient(const ConnectionState&);

    const ClientInfo& getClient(const ConnectionState&) const;
    const ClientInfo& getClient(std::int32_t) const;

private:
    sf::Image m_defaultImage;
    std::array<ClientInfo, 4u> m_clientInfo;
    void getSteamInfo(ClientInfo&);
};