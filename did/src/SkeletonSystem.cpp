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

#include "SkeletonSystem.hpp"
#include "MessageIDs.hpp"
#include "Actor.hpp"
#include "InventorySystem.hpp"
#include "AnimationSystem.hpp"
#include "QuadTreeFilter.hpp"
#include "CollisionBounds.hpp"
#include "GlobalConsts.hpp"
#include "ServerRandom.hpp"
#include "ServerSharedStateData.hpp"
#include "PacketTypes.hpp"
#include "Packet.hpp"
#include "Server.hpp"
#include "PathFinder.hpp"
#include "Operators.hpp"
#include "ServerRandom.hpp"
#include "CarriableSystem.hpp"
#include "BoatSystem.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Random.hpp>

#include <array>

#ifdef XY_DEBUG
#define SEND_DEBUG DebugState state; \
state.serverID = entity.getIndex(); \
state.state = skeleton.state; \
state.pathSize = skeleton.pathPoints.size(); \
state.target = skeleton.target; \
m_sharedData.gameServer->broadcastData(PacketID::DebugUpdate, state/*, EP2PSend::k_EP2PSendReliable*/);
#else
#define SEND_DEBUG
#endif

namespace
{
    /*
    Spawning rules
    No more spawns per day night cycle than there are holes
    Spawn only at night

    Behaviour
    Wanders from random point to next using plotted path. If
    player comes within vicinity path plotting switches to
    player position as target rather than random, target.
    Always avoids lanterns

    for day/night cycle time mapping, see DayNightSystem.cpp
    */

    std::array<float, 10> spawnSpacing = 
    {
        9.f, 7.f, 12.8f, 13.7f, 7.f, 9.f, 7.1f, 8.f, 12.6f, 13.7f
    };

    const float Speed = 36.f;
}

SkeletonSystem::SkeletonSystem(xy::MessageBus& mb, Server::SharedStateData& sd, PathFinder& pf)
    : xy::System        (mb, typeid(SkeletonSystem)),
    m_sharedData        (sd),
    m_pathFinder        (pf),
    m_dayPosition       (0.8f),
    m_spawnTimeIndex    (0),
    m_spawnPositionIndex(0),
    m_spawnCount        (0),
    m_spawnTime         (0)
{
    requireComponent<Skeleton>();
    requireComponent<Inventory>();
    requireComponent<xy::Transform>();
}

//public
void SkeletonSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::MapMessage)
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::HoleAdded)
        {
            m_spawnPoints.push_back(data.position);
        }
        else if (data.type == MapEvent::DayNightUpdate)
        {
            //track cycle so only spawn at night
            m_dayPosition = data.value;
        }
    }
}

void SkeletonSystem::process(float dt)
{
    //check spawn time
    m_spawnTime += dt;
    if (m_spawnTime > spawnSpacing[m_spawnTimeIndex])
    {
        m_spawnTime = 0.f;
        m_spawnTimeIndex = (m_spawnTimeIndex + 1) % spawnSpacing.size();

        if(!isDayTime())
        {
            if (!m_spawnPoints.empty() && m_spawnCount < (m_spawnPoints.size() / 2))
            {
                auto position = m_spawnPoints[m_spawnPositionIndex];
                m_spawnPositionIndex = (m_spawnPositionIndex + 1) % m_spawnPoints.size();

                auto area = sf::FloatRect(position, sf::Vector2f(Global::TileSize, Global::TileSize));

                //check no lanterns nearby
                bool canSpawn = true;
                const auto lights = getScene()->getSystem<xy::DynamicTreeSystem>().query(area, QuadTreeFilter::Light);
                for (auto light : lights) 
                {
                    auto lightPos = light.getComponent<xy::Transform>().getWorldPosition();
                    auto len = xy::Util::Vector::lengthSquared(lightPos - position);
                    if (len < (Global::LightRadius * Global::LightRadius))
                    {
                        canSpawn = false;
                        break;
                    }
                }

                if (canSpawn)
                {
                    //send spawn message
                    auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                    msg->position = position;
                    msg->type = ActorEvent::RequestSpawn;
                    msg->id = Actor::ID::Skeleton;
                }
            }
        }
    }

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& skeleton = entity.getComponent<Skeleton>();
        switch (skeleton.state)
        {
        default:
        case Skeleton::Dying:
            updateDying(entity, dt);
            break;
        case Skeleton::Spawning:
            updateSpawning(entity, dt);
            break;
        case Skeleton::Walking:
            updateNormal(entity, dt);

            skeleton.scanTime += dt;
            if (skeleton.scanTime > 1.f
                && !skeleton.fleeingLight)
            {
                skeleton.scanTime = 0.f;
                setPathToTreasure(entity);

                SEND_DEBUG;
            }
            break;
        //case Skeleton::Thinking:
            //updateThinking(entity, dt);
            //break;
        }
    }
}

//private
void SkeletonSystem::updateSpawning(xy::Entity entity, float dt)
{
    auto& skeleton = entity.getComponent<Skeleton>();
    skeleton.timer -= dt;
    if (skeleton.timer < 0)
    {
        skeleton.state = Skeleton::Walking;
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Skeleton | QuadTreeFilter::BotQuery);

        //random path to start
        setRandomPath(entity);
    }
}

void SkeletonSystem::updateNormal(xy::Entity entity, float dt)
{
    auto& skeleton = entity.getComponent<Skeleton>();
    auto& tx = entity.getComponent<xy::Transform>();
    auto& animationController = entity.getComponent<AnimationModifier>();
    auto& actor = entity.getComponent<Actor>();

    //update movement / AI
    if (!skeleton.pathPoints.empty())
    {
        skeleton.pathRequested = false;

        //TODO is there a way to not normalise this every frame?
        auto direction = skeleton.pathPoints.back() - tx.getPosition();
        auto dist2 = xy::Util::Vector::lengthSquared(direction);
        if (dist2 > 25.f)
        {
            auto len = std::sqrt(dist2);
            direction /= len;
            tx.move(direction * Speed * dt);

            if (direction.y > 0)
            {
                animationController.nextAnimation = AnimationID::WalkDown;
                actor.direction = Player::Direction::Down;
            }
            else if (direction.y < 0)
            {
                animationController.nextAnimation = AnimationID::WalkUp;
                actor.direction = Player::Up;
            }

            //only switch to side anims when longest part of vector
            if (direction.x > 0 && direction.x > std::abs(direction.y))
            {
                animationController.nextAnimation = AnimationID::WalkRight;
                actor.direction = Player::Right;
            }
            else if (direction.x < 0 && direction.x < -std::abs(direction.y))
            {
                animationController.nextAnimation = AnimationID::WalkLeft;
                actor.direction = Player::Left;
            }
        }
        else
        {
            skeleton.pathPoints.pop_back();
            if (skeleton.pathPoints.empty())
            {
                animationController.nextAnimation = AnimationID::IdleDown;
                actor.direction = Player::Down;

                skeleton.target = Skeleton::None;
                skeleton.targetEntity = {};
                skeleton.fleeingLight = false;

                //try finding a path to a recovered chest - we want's to grab it!
                if (!setPathToTreasure(entity))
                {
                    //else go about our random business
                    setRandomPath(entity);
                }
            }
        }
    }
    else
    {
        if (!setPathToTreasure(entity))
        {
            setRandomPath(entity);
        }
        skeleton.fleeingLight = false;
    }

    //if target type is treasure check if no one
    //has picked it up, and if it is in range for us to pick up
    if (skeleton.target == Skeleton::Treasure)
    {
        auto direction = tx.getPosition() - skeleton.targetEntity.getComponent<xy::Transform>().getPosition();
        auto len2 = xy::Util::Vector::lengthSquared(direction);
        if (len2 < 256)
        {
            bool stillValid = false;
            if (skeleton.targetEntity.getComponent<Actor>().id == Actor::Boat)
            {
                stillValid = skeleton.targetEntity.getComponent<Boat>().treasureCount > 0;
            }
            else //must be carriable
            {
                stillValid = skeleton.targetEntity.isValid() && !skeleton.targetEntity.getComponent<Carriable>().carried;
            }

            if (stillValid)
            {
                //try grabbing
                //TODO use a timer to not spam this every frame
                if ((entity.getComponent<Carrier>().carryFlags & Carrier::Treasure) == 0)
                {
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->entity = entity;
                    msg->action = PlayerEvent::WantsToCarry;

                    //std::cout << "skelly grab treasure\n";
                }
            }
            else
            {
                //target lost, reset
                skeleton.target = Skeleton::None;
                skeleton.targetEntity = {};

                if (!setPathToTreasure(entity))
                {
                    setRandomPath(entity);
                }
            }
        }
    }
    //else try following player
    else if (skeleton.target == Skeleton::Player
        && !skeleton.pathRequested)
    {
        if (skeleton.pathPoints.empty() || skeleton.targetEntity.destroyed()) //might be a decoy!
        {
            skeleton.target = Skeleton::None;
            skeleton.targetEntity = {};
        }
        else
        {
            auto targetPos = skeleton.targetEntity.getComponent<xy::Transform>().getPosition();
            auto direction = tx.getPosition() - targetPos;
            auto len2 = xy::Util::Vector::lengthSquared(direction);
            if (len2 > 100000) //a bit less than 10 tiles
            {
                skeleton.pathPoints.clear();
                skeleton.target = Skeleton::None;
                skeleton.targetEntity = {};
            }
            else if (len2 < 1024)
            {
                if (skeleton.pathPoints.size() == 1)
                {
                    skeleton.pathPoints.push_back(targetPos);
                }
            }
        }
    }

    updateCollision(entity);

    //recoil when hit
    auto len2 = xy::Util::Vector::lengthSquared(skeleton.velocity);
    if (len2 > 5)
    {
        tx.move(skeleton.velocity * dt);
        skeleton.velocity *= 0.89f;
    }
    else
    {
        skeleton.velocity = {};
    }

    //set fire to skeletons in daylight
    if (isDayTime() && !skeleton.onFire)
    {
        skeleton.onFire = true;
        skeleton.timer = 1.f;

        //broadcast new entity to clients - this doesn't technically exist server side
        ActorState state;
        state.actorID = Actor::ID::Fire;
        state.position = tx.getPosition();
        state.serverID = entity.getIndex(); //client makes a special case to attach to this actor
        m_sharedData.gameServer->broadcastData(PacketID::ActorData, state, xy::NetFlag::Reliable, Global::ReliableChannel);
    }

    if (skeleton.onFire || skeleton.inLight)
    {
        skeleton.timer -= dt;
        if (skeleton.timer < 0)
        {
            skeleton.timer = 1.f;
            entity.getComponent<Inventory>().lastDamager = -1;

            auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
            msg->entity = entity;
            msg->action = PlayerEvent::PistolHit;
        }
    }

    if (entity.getComponent<Inventory>().health == 0)
    {
        //allow time for dying animation
        skeleton.state = Skeleton::Dying;
        skeleton.timer = 0.8f;

        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;

        //drop anything we might be carrying
        if ((entity.getComponent<Carrier>().carryFlags & Carrier::Treasure))
        {
            auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
            msg->entity = entity;
            msg->action = PlayerEvent::DroppedCarrying;
        }
    }
}

void SkeletonSystem::updateDying(xy::Entity entity, float dt)
{
    auto& skeleton = entity.getComponent<Skeleton>();
    skeleton.timer -= dt;
    if (skeleton.timer < 0)
    {
        auto position = entity.getComponent<xy::Transform>().getPosition();

        //chance to spawn a coin or two
        auto chance = Server::getRandomInt(0, 3);
        if (chance == 0 && /*!isDayTime() &&*/ entity.getComponent<CollisionComponent>().water == 0)
        {
            auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
            msg->id = Actor::ID::Coin;
            msg->position = position;
            msg->type = ActorEvent::RequestSpawn;

            //second coin if we're lucky
            if (xy::Util::Random::value(0, 5) == 0)
            {
                msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                msg->id = Actor::ID::Coin;
                msg->position = position;
                msg->type = ActorEvent::RequestSpawn;
            }
        }

        //raise actor message saying skeleton died
        //with actor id of last damager as data
        auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
        msg->id = Actor::ID::Skeleton;
        msg->position = position;
        msg->type = ActorEvent::Died;
        msg->data = entity.getComponent<Inventory>().lastDamager;

        getScene()->destroyEntity(entity);
    }
}

//void SkeletonSystem::updateThinking(xy::Entity entity, float dt)
//{
//    LOG("Skeleton Thinking", xy::Logger::Type::Info);
//
//    auto & skeleton = entity.getComponent<Skeleton>();
//    skeleton.timer -= dt;
//    updateCollision(entity);
//
//    if (skeleton.timer < 0)
//    {
//        skeleton.state = Skeleton::Walking;
//
//        //random path to start
//        setRandomPath(entity);
//
//        SEND_DEBUG;
//    }
//
//    entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
//
//    if (entity.getComponent<Inventory>().health == 0)
//    {
//        //allow time for dying animation
//        skeleton.state = Skeleton::Dying;
//        skeleton.timer = 0.8f;
//
//        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);
//        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;
//
//        SEND_DEBUG;
//    }
//}

void SkeletonSystem::updateCollision(xy::Entity entity)
{
    const auto& collision = entity.getComponent<CollisionComponent>();
    for (auto i = 0u; i < collision.manifoldCount; ++i)
    {
        auto& skeleton = entity.getComponent<Skeleton>();

        const auto& manifold = collision.manifolds[i];
        switch (manifold.ID)
        {
        default: break;
        case ManifoldID::Player:
            entity.getComponent<Inventory>().lastDamager = manifold.otherEntity.getComponent<Actor>().id;
            
            skeleton.velocity += manifold.normal * 140.f;
            {
                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->entity = entity;
                msg->action = PlayerEvent::PistolHit;
            }
            break;
        case ManifoldID::Explosion:
            skeleton.state = Skeleton::Dying;
            skeleton.timer = 1.2f;
            entity.getComponent<Inventory>().lastDamager = -1;

            entity.getComponent<Inventory>().health = 0;
            entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);
            entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;
            entity.getComponent<xy::Transform>().move(manifold.penetration * manifold.normal);
            break;
        case ManifoldID::Terrain:
            entity.getComponent<xy::Transform>().move(manifold.penetration * manifold.normal);

            //this backs skelly away and forces a new path calc
            skeleton.pathPoints.clear();
            skeleton.pathPoints.push_back(entity.getComponent<xy::Transform>().getPosition() + (manifold.normal * (Global::TileSize * 2.5f)));
            skeleton.target = Skeleton::None;
            skeleton.targetEntity = {};
            entity.getComponent<Inventory>().lastDamager = -1;
            break;
        }
    }

    if (collision.water > 0) //water hurts skeletons
    {
        auto* newMsg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
        newMsg->action = PlayerEvent::PistolHit;
        newMsg->entity = entity;
    }

    sf::FloatRect bounds = { -Global::LightRadius, -Global::LightRadius, Global::LightRadius * 2.f, Global::LightRadius*2.f };
    auto position = entity.getComponent<xy::Transform>().getPosition();
    bounds.left += position.x;
    bounds.top += position.y;

    auto lights = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds, QuadTreeFilter::Light);
    for (auto light : lights)
    {
        const float lightRadSqr = Global::LightRadius * Global::LightRadius;
        auto dir = position - light.getComponent<xy::Transform>().getPosition();
        float len2 = xy::Util::Vector::lengthSquared(dir);
        auto& skeleton = entity.getComponent<Skeleton>();
        skeleton.inLight = len2 < lightRadSqr;

        if (skeleton.inLight && !skeleton.fleeingLight)
        {
            skeleton.fleeingLight = true;

            //plot a path in the opposite direction
            if (!skeleton.pathRequested)
            {
                skeleton.pathPoints.clear();
                skeleton.target = Skeleton::None;
                skeleton.targetEntity = {};

                auto dest = position + (dir * 3.f);

                sf::Vector2i start(static_cast<int>(position.x / Global::TileSize), static_cast<int>(position.y / Global::TileSize));
                sf::Vector2i end(static_cast<int>(dest.x / Global::TileSize), static_cast<int>(dest.y / Global::TileSize));
                end.x = xy::Util::Math::clamp(end.x, 6, static_cast<int>(Global::TileCountX) - 12);
                end.x = xy::Util::Math::clamp(end.y, 6, static_cast<int>(Global::TileCountY) - 12);

                m_pathFinder.plotPathAsync(start, end, skeleton.pathPoints);
                skeleton.pathRequested = true;
            }
        }
    }
}

bool SkeletonSystem::setPathToTreasure(xy::Entity entity)
{
    auto pos = entity.getComponent<xy::Transform>().getPosition();
    auto& skeleton = entity.getComponent<Skeleton>();

    if (skeleton.pathRequested || skeleton.target != Skeleton::None) return false;

    sf::FloatRect queryBounds(-Global::TileSize * 3.f, -Global::TileSize * 3.f, Global::TileSize * 6.f, Global::TileSize * 6.f);
    queryBounds.left += pos.x;
    queryBounds.top += pos.y;

    static const auto filter = QuadTreeFilter::Decoy | QuadTreeFilter::Player | QuadTreeFilter::Treasure | QuadTreeFilter::Boat;
    auto entities = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryBounds, filter);
    
    //prioritise decoy and treasure over player
    std::sort(entities.begin(), entities.end(),
        [](xy::Entity a, xy::Entity b)
    {
        return a.getComponent<Actor>().id > b.getComponent<Actor>().id;
    });

    for (auto e : entities)
    {
        const auto& actor = e.getComponent<Actor>();
        if ((actor.id == Actor::ID::Treasure && !e.getComponent<Carriable>().carried)
            || (actor.id == Actor::Boat && e.getComponent<Boat>().treasureCount > 0))
        {
            auto treasurePos = e.getComponent<xy::Transform>().getPosition();

            sf::Vector2i start(static_cast<int>(pos.x / Global::TileSize), static_cast<int>(pos.y / Global::TileSize));
            sf::Vector2i end(static_cast<int>(treasurePos.x / Global::TileSize), static_cast<int>(treasurePos.y / Global::TileSize));
            skeleton.pathPoints.clear();
            m_pathFinder.plotPathAsync(start, end, skeleton.pathPoints);
            skeleton.pathRequested = true;

            skeleton.target = Skeleton::Treasure;
            skeleton.targetEntity = e;
            //LOG("Skeleton Targeting Treasure", xy::Logger::Type::Info);
            return true;
        }
        else if ((actor.id >= Actor::ID::PlayerOne && actor.id <= Actor::ID::PlayerFour)
            || actor.id == Actor::Decoy)
        {
            //follow player around
            auto playerPos = e.getComponent<xy::Transform>().getPosition();

            sf::Vector2i start(static_cast<int>(pos.x / Global::TileSize), static_cast<int>(pos.y / Global::TileSize));
            sf::Vector2i end(static_cast<int>(playerPos.x / Global::TileSize), static_cast<int>(playerPos.y / Global::TileSize));
            m_pathFinder.plotPathAsync(start, end, skeleton.pathPoints);
            skeleton.pathPoints.clear();
            skeleton.pathRequested = true;

            skeleton.target = Skeleton::Player;
            skeleton.targetEntity = e;
            //LOG("Skeleton Targeting Player " + std::to_string(actor.id), xy::Logger::Type::Info);
            return true;
        }
    }

    return false;
}

void SkeletonSystem::setRandomPath(xy::Entity entity)
{
    auto pos = entity.getComponent<xy::Transform>().getPosition();
    auto& skeleton = entity.getComponent<Skeleton>();

    if (skeleton.pathRequested || !skeleton.pathPoints.empty()) return;

    sf::Vector2i start(static_cast<int>(pos.x / Global::TileSize), static_cast<int>(pos.y / Global::TileSize));
    sf::Vector2i end(Server::getRandomInt(8, Global::TileCountX - 16), Server::getRandomInt(8, Global::TileCountY - 16));
    m_pathFinder.plotPathAsync(start, end, skeleton.pathPoints);
    skeleton.pathRequested = true;
    LOG("Skeleton got random path", xy::Logger::Type::Info);
}

bool SkeletonSystem::isDayTime() const
{
    return (m_dayPosition > 0.25f && m_dayPosition < 0.5f) || (m_dayPosition > 0.75f && m_dayPosition < 1.f);
}

void SkeletonSystem::onEntityAdded(xy::Entity)
{
    m_spawnCount++;
}

void SkeletonSystem::onEntityRemoved(xy::Entity)
{
    m_spawnCount--;
    m_spawnTime = 0.f;
}