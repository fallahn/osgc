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
    std::uint64_t peerID = 0;
    bool ready = true;
    std::uint32_t score = 0; //this is updated by the round summary
    std::uint32_t gamesPlayed = 0; //as is this
};

class ClientInfoManager final
{
public:
    ClientInfoManager();

    const ClientInfo& getClient(std::int32_t) const;
    ClientInfo& getClient(std::int32_t);

    void resetClient(std::int32_t);

    void setHostID(std::uint64_t id) { m_host = id; }
    std::uint64_t getHostID() const { return m_host; }

private:
    std::array<ClientInfo, 4u> m_clientInfo;
    std::uint64_t m_host;
};