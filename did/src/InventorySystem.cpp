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

#include "InventorySystem.hpp"
#include "Server.hpp"
#include "MessageIDs.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"
#include "ServerRandom.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/ecs/Entity.hpp>

namespace
{
    const std::int32_t SwordDamage = 1;
    const std::int32_t PistolDamage = 4;
    const std::int32_t BeeDamage = 3;
    const std::int32_t SkellyDamage = 4;
    const std::int32_t ExplosionDamage = 8;
    const std::int32_t DrownDamage = 8;
}

InventorySystem::InventorySystem(xy::MessageBus& mb, Server::SharedStateData& gs)
    : xy::System(mb, typeid(InventorySystem)),
    m_sharedData(gs)
{
    requireComponent<Inventory>();
}

//public
void InventorySystem::handleMessage(const xy::Message& msg)
{
    auto doDamage = [](std::int32_t damage, const PlayerEvent& data)
    {
        auto e = data.entity;
        auto& inventory = e.getComponent<Inventory>();
        inventory.health = std::max(0, inventory.health - damage);
        inventory.sendUpdate = true;
        inventory.lastDamage = data.action;
        inventory.lastDamager = data.data;
    };

    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        switch (data.action)
        {
        default: break;
        case PlayerEvent::NextWeapon:
            switchWeapon(1, data.entity);
            break;
        case PlayerEvent::PreviousWeapon:
            switchWeapon(-1, data.entity);
            break;
        case PlayerEvent::DidAction:
        {
            auto e = data.entity;

            auto& inventory = e.getComponent<Inventory>();
            if (inventory.weapon == Inventory::Pistol)
            {
                inventory.ammo = std::max(0, inventory.ammo - 1);
                
                //send update to client if not a bot
                if (e.getComponent<std::uint64_t>() > 0)
                {
                    InventoryState state;
                    state.inventory = inventory;
                    state.parentID = e.getIndex();

                    m_sharedData.gameServer->sendData(PacketID::InventoryUpdate, state, e.getComponent<std::uint64_t>(), xy::NetFlag::Reliable);
                }
            }
        }
        break;
        case PlayerEvent::SwordHit:
            doDamage(SwordDamage, data);
            break;
        case PlayerEvent::PistolHit:
            doDamage(PistolDamage, data);
            break;
        case PlayerEvent::TouchedSkelly:
            doDamage(SkellyDamage, data);
            break;
        case PlayerEvent::ExplosionHit:
            doDamage(ExplosionDamage, data);
            break;
        case PlayerEvent::Drowned:
            doDamage(DrownDamage, data);
            break;
        case PlayerEvent::BeeHit:
            doDamage(BeeDamage, data);
            break;
        case PlayerEvent::Respawned:
        {
            auto e = data.entity;
            auto& inventory = e.getComponent<Inventory>();
            inventory.health = Inventory::MaxHealth;
            inventory.sendUpdate = true;
        }
            break;
        case PlayerEvent::StashedTreasure:
        {
            auto e = data.entity;
            auto& inventory = e.getComponent<Inventory>();
            inventory.treasure += Global::TreasureValue;// Server::getRandomInt(2, 5) * 10;
            inventory.sendUpdate = true;
        }
            break;

        case PlayerEvent::StoleTreasure:
        {
            for (auto entity : getEntities())
            {
                if (entity.hasComponent<Player>() &&
                    entity.getComponent<Player>().playerNumber == data.data)
                {
                    auto& inventory = entity.getComponent<Inventory>();
                    inventory.treasure -= Global::TreasureValue;
                    inventory.sendUpdate = true;

                    break;
                }
            }
        }
            break;
        }
    }
}

void InventorySystem::process(float)
{
    auto& entities = getEntities();

    for(auto entity : entities)
    {
        auto& inventory = entity.getComponent<Inventory>();
        if (inventory.sendUpdate)
        {
            sendUpdate(inventory, entity.getIndex());
        }
    }

}

//private
void InventorySystem::switchWeapon(sf::Int32 next, xy::Entity entity)
{
    auto& inventory = entity.getComponent<Inventory>();
    inventory.weapon += next;
    
    if(inventory.weapon < 0) inventory.weapon += Inventory::Count;
    else inventory.weapon %= Inventory::Count;

    //needs to be sent to everyone so client can
    //update weapon sprites of other players
    inventory.sendUpdate = true;
}

void InventorySystem::sendUpdate(Inventory& inventory, std::uint32_t parentID)
{
    InventoryState state;
    state.inventory = inventory;
    state.parentID = parentID;

    m_sharedData.gameServer->broadcastData(PacketID::InventoryUpdate, state, xy::NetFlag::Reliable);

    inventory.sendUpdate = false;
}