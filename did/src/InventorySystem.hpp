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

#include "ServerSharedStateData.hpp"

#include <xyginext/ecs/System.hpp>

struct Inventory final
{
    static constexpr sf::Int8 MaxHealth = 25;
    static constexpr sf::Uint8 MaxAmmo = 5;

    sf::Uint16 treasure = 0;
    sf::Uint8 ammo = MaxAmmo;
    enum Weapon
    {
        Pistol, Sword, Shovel, Count
    };
    sf::Int8 weapon = Weapon::Shovel;
    sf::Int8 health = MaxHealth;
    sf::Int8 lastDamage = -1; //last action to cause damage
    sf::Int8 lastDamager = -1; //actor to cause damage

    static constexpr float PistolCoolDown = 1.5f;
    static constexpr float SwordCoolDown = 0.375f;
    static constexpr float ShovelCoolDown = 0.5f;

    bool sendUpdate = false;
    void reset()
    {
        treasure = 0;
        ammo = MaxAmmo;
        health = MaxHealth;
        sendUpdate = true;
    }
};

class GameServer;
class InventorySystem final : public xy::System
{
public:
    InventorySystem(xy::MessageBus&, Server::SharedStateData&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

private:
    Server::SharedStateData& m_sharedData;

    void switchWeapon(sf::Int32, xy::Entity);
    void sendUpdate(Inventory&, std::uint32_t);
};