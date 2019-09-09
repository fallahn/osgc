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

#include "CollectibleSystem.hpp"
#include "InventorySystem.hpp"
#include "CollisionBounds.hpp"
#include "ServerSharedStateData.hpp"
#include "PacketTypes.hpp"
#include "Packet.hpp"
#include "Server.hpp"
#include "ServerRandom.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>

namespace
{

}

CollectibleSystem::CollectibleSystem(xy::MessageBus& mb, Server::SharedStateData& sd)
    : xy::System(mb, typeid(CollectibleSystem)),
    m_sharedData(sd)
{
    requireComponent<Collectible>();
    requireComponent<CollisionComponent>();
    requireComponent<xy::Transform>();
}

//public
void CollectibleSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& collectible = entity.getComponent<Collectible>();
        if (collectible.collectionTime > 0)
        {
            collectible.collectionTime -= dt;
        }
        else
        {
            //check for collision with player and award if found
            const auto& collision = entity.getComponent<CollisionComponent>();
            for(auto i = 0u; i < collision.manifoldCount; ++i)
            {
                if (collision.manifolds[i].ID == ManifoldID::Player)
                {
                    auto otherEnt = collision.manifolds[i].otherEntity;
                    auto& inventory = otherEnt.getComponent<Inventory>();

                    bool collected = false;

                    switch (collectible.type)
                    {
                    default: break;
                    case Collectible::Type::Ammo:
                        if (inventory.ammo < Inventory::MaxAmmo)
                        {
                            inventory.ammo = std::min(Inventory::MaxAmmo, static_cast<sf::Uint8>(inventory.ammo + collectible.value));
                            collected = true;
                        }
                        break;
                    case Collectible::Type::Coin:
                        inventory.treasure += collectible.value;
                        collected = true;
                        break;
                    case Collectible::Type::Food:
                        if (inventory.health < Inventory::MaxHealth)
                        {
                            inventory.health = std::min(Inventory::MaxHealth, static_cast<sf::Int8>(inventory.health + collectible.value));
                            collected = true;
                        }
                        break;
                    }
                    //TODO other types

                    if (collected)
                    {
                        //only send if we're not a bot player
                        if (otherEnt.getComponent<std::uint64_t>() != 0)
                        {
                            InventoryState state;
                            state.inventory = inventory;
                            state.parentID = otherEnt.getIndex();
                            m_sharedData.gameServer->sendData(PacketID::InventoryUpdate, state, otherEnt.getComponent<std::uint64_t>(), xy::NetFlag::Reliable);
                        }

                        getScene()->destroyEntity(entity);

                        auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                        msg->type = ActorEvent::Died;

                        switch (collectible.type)
                        {
                        default: break;
                        case Collectible::Type::Ammo:
                            msg->id = Actor::ID::Ammo;
                            break;
                        case Collectible::Type::Coin:
                            msg->id = Actor::ID::Coin;
                            break;
                        case Collectible::Type::Food:
                            msg->id = Actor::ID::Food;
                            break;
                        }
                    }

                    break;
                }
            }
        }

        //despawn health items after a while
        //if (collectible.type == Collectible::Type::Food)
        {
            collectible.lifeTime -= dt;
            if (collectible.lifeTime < 0)
            {
                getScene()->destroyEntity(entity);

                auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                msg->type = ActorEvent::Died;

                switch (collectible.type)
                {
                default: break;
                case Collectible::Type::Ammo:
                    msg->id = Actor::ID::Ammo;
                    break;
                case Collectible::Type::Coin:
                    msg->id = Actor::ID::Coin;
                    break;
                case Collectible::Type::Food:
                    msg->id = Actor::ID::Food;
                    break;
                }
            }
        }

        entity.getComponent<xy::Transform>().move(collectible.velocity * dt);
        collectible.velocity *= 0.9f;
    }
}

//private
void CollectibleSystem::onEntityAdded(xy::Entity entity)
{
    auto& collectible = entity.getComponent<Collectible>();
    collectible.velocity.x = Server::getRandomFloat(-100.f, 100.f);
    collectible.velocity.y = Server::getRandomFloat(-100.f, 100.f);
}