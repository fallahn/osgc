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

#include "InventorySystem.hpp"

#include <xyginext/ecs/System.hpp>

struct Barrel final
{
    float speed = 60.f;
    sf::Vector2f velocity;
    float despawnTime = 0.75f;

    enum
    {
        Floating,
        Beached,
        Breaking
    }state = Floating;
    
    //map these values so we can use the inventory to store barrel contents
    enum 
    {
        Gold = Inventory::Shovel,
        Item = Inventory::Sword, //items are chosen by BarrelSytem when barrel is broken
        Explosive = Inventory::Pistol
    };
};

namespace Server
{
    struct SharedStateData;
}

class BarrelSystem final : public xy::System
{
public:
    BarrelSystem(xy::MessageBus&, Server::SharedStateData&);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:
    Server::SharedStateData& m_sharedData;

    std::size_t m_spawnIndex;
    float m_spawnTimer;

    std::size_t m_itemIndex;

    std::size_t m_mineCount;
    static constexpr std::size_t MaxMines = 10;

    void spawn();
    void updateFloating(xy::Entity, float);
    void updateBeached(xy::Entity);
    void updateBreaking(xy::Entity, float);
};
