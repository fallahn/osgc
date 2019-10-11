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

#include <xyginext/ecs/System.hpp>

#include <array>

//enables an entity to pick things up
struct Carrier final
{
    enum Flags
    {
        //as these have to be sync'd as part of the player
        //component the flags start at 0x10. Be wary of updating
        //values in the player flags enum, and make sure that
        //if we go beyond 12 values here that we increase the
        //player flags word to accomodate.
        Treasure = 0x10,
        Decoy = 0x20,
        Flare = 0x40,
        Magnet = 0x80,
        SpookySkull = 0x100,
        Mine = 0x200,
        Torch = 0x400,
    };

    std::int32_t carryFlags = 0;
    xy::Entity carriedEntity;
};

struct Carriable final
{
    static constexpr float FlareOffset = 0.75f;

    bool carried = false;
    bool stashed = false; //in a boat and could be stolen...
    sf::Vector2f spawnPosition; //carriables dropped in the water return here
    float offsetMultiplier = 1.f; //allows carrying smaller items closer to player
    xy::Entity parentEntity; //REMEMBER nullify this when dropped
    std::uint16_t type = 0; //player flag to set or unset when carrying this
    
    std::function<void(xy::Entity, std::uint8_t)> action; //performed if this is a consumable item like a decoy, with ID of performer
};

namespace Server
{
    struct SharedStateData;
}

class CarriableSystem final : public xy::System
{
public:
    CarriableSystem(xy::MessageBus&, Server::SharedStateData&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

    void onEntityAdded(xy::Entity) override;

private:

    Server::SharedStateData& m_sharedData;
    void tryGrab(xy::Entity);
    void tryDrop(xy::Entity, bool = false);

    std::array<xy::Entity, 8u> m_stashedTreasures;
    std::size_t m_stashedIndex;
};

sf::Vector2f operator * (sf::Vector2f l, sf::Vector2f r);
