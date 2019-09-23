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

#include "BarrelSystem.hpp"
#include "SharedStateData.hpp"
#include "AnimationSystem.hpp"
#include "CollisionBounds.hpp"
#include "ServerRandom.hpp"
#include "Server.hpp"
#include "GlobalConsts.hpp"
#include "MessageIDs.hpp"
#include "Actor.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <array>
#include <algorithm>

/*
Barrels are spawned out at sea and drift onto the beach.
They maybe be explosive or contain food or coins. Or poop snails
*/

namespace
{
    const std::size_t MaxBarrels = 8;
    const std::array<float, 10u> SpawnTimes =
    {
        20.f, 11.f, 20.f, 23.f, 19.f,
        21.f, 24.f, 19.f, 23.f, 20.f
    };

    //we shuffle this each time a new system is created
    std::array<Actor::ID, 14u> Items = 
    {
        Actor::Food, Actor::Food ,Actor::Food, Actor::MineItem,
        Actor::Ammo, Actor::Ammo, Actor::Ammo, Actor::MineItem,
        Actor::DecoyItem, Actor::DecoyItem,
        Actor::FlareItem, Actor::FlareItem,
        Actor::SkullItem, Actor::SkullItem
    };
}

BarrelSystem::BarrelSystem(xy::MessageBus& mb, Server::SharedStateData& sd)
    : xy::System(mb, typeid(BarrelSystem)),
    m_sharedData(sd),
    m_spawnIndex(0),
    m_spawnTimer(0.f),
    m_itemIndex (0)
{
    requireComponent<Barrel>();
    requireComponent<xy::Transform>();
    requireComponent<CollisionComponent>();

    std::shuffle(Items.begin(), Items.end(), Server::rndEngine);
}

//public
void BarrelSystem::handleMessage(const xy::Message& msg)
{

}

void BarrelSystem::process(float dt)
{
    auto& entities = getEntities();
    
    //check if it's time to spawn a barrel
    m_spawnTimer += dt;
    if (m_spawnTimer > SpawnTimes[m_spawnIndex])
    {
        m_spawnIndex = (m_spawnIndex + 1) % SpawnTimes.size();
        m_spawnTimer = 0.f;

        if (entities.size() < MaxBarrels)
        {
            spawn();
        }
    }

    //update existing barrels
    for (auto entity : entities)
    {
        const auto& barrel = entity.getComponent<Barrel>();
        switch (barrel.state)
        {
        case Barrel::Floating:
            updateFloating(entity, dt);
            break;
        case Barrel::Beached:
            updateBeached(entity);
            break;
        case Barrel::Breaking:
            updateBreaking(entity, dt);
            break;
        }
    }
}

//private
void BarrelSystem::spawn()
{
    auto direction = Server::getRandomInt(0, 3);
    sf::Vector2f position;
    switch (direction)
    {
    case 0:
        position.x = 1.f;
        break;
    case 1:
        position.x = -1.f;
        break;
    case 2:
        position.y = 1.f;
        break;
    case 3:
        position.y = -1.f;
        break;
    }

    auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
    msg->position = position; //actually velocity
    msg->type = ActorEvent::RequestSpawn;
    msg->id = Actor::ID::Barrel;
}

void BarrelSystem::updateFloating(xy::Entity entity, float dt)
{
    auto& barrel = entity.getComponent<Barrel>();
    auto& tx = entity.getComponent<xy::Transform>();
    const auto& collision = entity.getComponent<CollisionComponent>();

    tx.move(barrel.velocity * barrel.speed * dt);

    if (collision.water == 0)
    {
        barrel.speed *= 0.97f;
        if (barrel.speed < 1)
        {
            barrel.state = Barrel::Beached;
            //stop sending position updates
            entity.getComponent<Actor>().nonStatic = false;
        }
    }
}

void BarrelSystem::updateBeached(xy::Entity entity)
{
    //check health and spawn explosion/item as necessary
    const auto& inventory = entity.getComponent<Inventory>();
    if (inventory.health == 0)
    {        
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);

        auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
        msg->position = entity.getComponent<xy::Transform>().getPosition();
        msg->type = ActorEvent::RequestSpawn;

        auto& barrel = entity.getComponent<Barrel>();
        barrel.state = Barrel::Breaking;
        if (inventory.weapon == Barrel::Explosive)
        {
            entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Explode;
            msg->id = Actor::Explosion;
        }
        else
        {
            entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Break;
    
            msg->id = inventory.weapon == Barrel::Gold ?
                Actor::ID::Coin : Items[m_itemIndex];

            m_itemIndex = (m_itemIndex + 1) % Items.size();

            //rarely a poop snail
            if (Server::getRandomInt(0, 16) == 0)
            {
                msg->id = Actor::ID::Crab;
            }
        }
    }
}

void BarrelSystem::updateBreaking(xy::Entity entity, float dt)
{
    auto& barrel = entity.getComponent<Barrel>();
    barrel.despawnTime -= dt;
    if (barrel.despawnTime < 0)
    {
        getScene()->destroyEntity(entity);
    }
}