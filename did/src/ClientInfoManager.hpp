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
};

class ClientInfoManager final
{
public:
    ClientInfoManager();

    const ClientInfo& getClient(std::int32_t) const;
    ClientInfo& getClient(std::int32_t);

    void resetClient(std::int32_t);

private:
    sf::Image m_defaultImage;
    std::array<ClientInfo, 4u> m_clientInfo;
};