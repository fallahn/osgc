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

#include "CrateSystem.hpp"
#include "Player.hpp"
#include "Collision.hpp"
#include "SharedStateData.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    const float Gravity = 5999.f;
    const float TerminalVelocity = 12000.f;

    //TODO as with dropping items, if this is made more generic
    //move elsewhere - possible to game consts?
    const sf::Vector2f CarryOffset(18.f, -10.f);
}

CrateSystem::CrateSystem(xy::MessageBus& mb, SharedData& sd)
    : xy::System    (mb, typeid(CrateSystem)),
    m_sharedData    (sd)
{
    requireComponent<Crate>();
    requireComponent<xy::BroadphaseComponent>();
    requireComponent<xy::Transform>();
}

//public
void CrateSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.type == PlayerEvent::Shot
            && m_sharedData.inventory.ammo == 0)
        {
            //try picking up a crate (or dropping player has one)
            auto playerEnt = data.entity;
            auto& player = playerEnt.getComponent<Player>();

            if (player.carriedItem.isValid())
            {
                //drop it - TODO if this is extended to generic carriables
                //then this should probably be elsewhere...
                auto& crate = player.carriedItem.getComponent<Crate>();
                crate.state = Crate::Falling;

                auto& tx = playerEnt.getComponent<xy::Transform>();
                auto& crateTx = player.carriedItem.getComponent<xy::Transform>();
                auto position = crateTx.getWorldPosition();
                crateTx.setPosition(position);
                crateTx.setScale(tx.getScale().y, tx.getScale().y); //not a mistake, don't want negative x vals
                tx.removeChild(crateTx);

                player.carriedItem = {};
            }
            else
            {
                //scan area for crates and pick up
                auto& tx = playerEnt.getComponent<xy::Transform>();
                auto position = tx.getPosition();
                auto offset = CarryOffset;
                offset.x *= tx.getScale().x;
                offset.y *= tx.getScale().y;

                position += offset;
                //TODO constify
                sf::FloatRect queryArea(position.x - 32.f, position.y - 32.f, 64.f, 64.f);
                auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryArea, CollisionShape::Crate);
                for (auto crate : nearby)
                {
                    auto& crateTx = crate.getComponent<xy::Transform>();
                    auto bounds = crate.getComponent<xy::BroadphaseComponent>().getArea();
                    bounds = crateTx.getTransform().transformRect(bounds);

                    if (bounds.contains(position))
                    {
                        //pick up crate
                        crateTx.setPosition(CarryOffset/* - crateTx.getOrigin()*/);
                        crateTx.setScale(1.f, 1.f); //now getting scale from parent
                        tx.addChild(crateTx);

                        player.carriedItem = crate;
                        crate.getComponent<Crate>().state = Crate::Carried;
                        break;
                    }
                }
            }
        }
    }
    else if (msg.id == MessageID::StarMessage)
    {
        const auto& data = msg.getData<StarEvent>();
        if (data.entityHit.getComponent<CollisionBody>().shapes[0].type == CollisionShape::Crate)
        {
            kill(data.entityHit);
        }
    }
}

void CrateSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& crate = entity.getComponent<Crate>();
        switch (crate.state)
        {
        default: break;
        case Crate::Falling:
            updateFalling(entity, dt);
            DPRINT("State", "Falling");
            break;
        case Crate::Idle:
            updateIdle(entity, dt);
            DPRINT("State", "Idle");
            break;
        case Crate::Dead:
            updateDead(entity, dt);
            DPRINT("State", "Dead");
            break;
        }
    }
}

//private
void CrateSystem::updateFalling(xy::Entity entity, float dt)
{
    auto& crate = entity.getComponent<Crate>();
    crate.velocity.y = std::min(crate.velocity.y + Gravity * dt, TerminalVelocity);

    applyVelocity(entity, dt);
    doCollision(entity);

    //check which shapes have collision
    if (crate.state != Crate::Dead) //collision may have killed it
    {
        if (entity.getComponent<CollisionBody>().collisionFlags & (CollisionShape::Crate/* | CollisionShape::Foot*/))
        {
            //touched the ground
            crate.velocity = {};
            crate.state = Crate::Idle;

            //TODO raise a message
        }
    }
}

void CrateSystem::updateIdle(xy::Entity entity, float dt)
{
    applyVelocity(entity, dt);
    doCollision(entity);

    auto& crate = entity.getComponent<Crate>();
    if (crate.state != Crate::Dead)
    {
        //if there's no foot collision then must be pushed off the edge
        if ((entity.getComponent<CollisionBody>().collisionFlags & CollisionShape::Foot) == 0)
        {
            crate.state = Crate::Falling;
        }
    }
}

void CrateSystem::updateDead(xy::Entity entity, float dt)
{
    auto& crate = entity.getComponent<Crate>();
    crate.stateTime -= dt;
    if (crate.stateTime < 0)
    {
        crate.state = Crate::Falling;

        auto& tx = entity.getComponent<xy::Transform>();
        auto scale = tx.getScale();
        scale.x = scale.y;
        tx.setScale(scale);
        tx.setPosition(crate.spawnPosition);
    }
}

void CrateSystem::applyVelocity(xy::Entity entity, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& crate = entity.getComponent<Crate>();

    tx.move(crate.velocity * dt);
    crate.velocity.x *= Crate::Drag;
}

void CrateSystem::doCollision(xy::Entity entity)
{
    auto& tx = entity.getComponent<xy::Transform>();
    entity.getComponent<CollisionBody>().collisionFlags = 0;

    //broad phase
    auto area = entity.getComponent<xy::BroadphaseComponent>().getArea();

    auto queryArea = boundsToWorldSpace(area, tx);
    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryArea, CollisionGroup::CrateFlags);

    for (auto other : nearby)
    {
        if (other != entity)
        {
            auto otherBounds = boundsToWorldSpace(other.getComponent<xy::BroadphaseComponent>().getArea(), other.getComponent<xy::Transform>());

            if (otherBounds.intersects(queryArea))
            {
                resolveCollision(entity, other, otherBounds);
            }
        }
    }
    DPRINT("Nearby", std::to_string(nearby.size()));
}

void CrateSystem::resolveCollision(xy::Entity entity, xy::Entity other, sf::FloatRect otherBounds)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& collisionBody = entity.getComponent<CollisionBody>();
    auto& crate = entity.getComponent<Crate>();

    auto position = tx.getPosition();

    for (auto i = 0u; i < collisionBody.shapeCount; ++i)
    {
        auto bounds = collisionBody.shapes[i].aabb;
        bounds = boundsToWorldSpace(bounds, tx);

        if (auto manifold = intersectsAABB(bounds, otherBounds); manifold)
        {
            auto& otherBody = other.getComponent<CollisionBody>();

            //flag crate body as having a collision on this shape
            if (otherBody.shapes[0].type & CollisionShape::Solid)
            {
                collisionBody.collisionFlags |= collisionBody.shapes[i].type;
                //DPRINT("Type", std::to_string(collisionBody.shapes[i].type));
            }

            if (collisionBody.shapes[i].type == CollisionShape::Crate)
            {
                switch (otherBody.shapes[0].type)
                {
                default: break;
                case CollisionShape::Solid:
                    tx.move(manifold->normal * manifold->penetration);

                    if (manifold->normal.y != 0)
                    {
                        crate.velocity = {};
                    }
                    else
                    {
                        crate.velocity = xy::Util::Vector::reflect(crate.velocity, manifold->normal);
                    }
                    break;
                case CollisionShape::Fluid:
                case CollisionShape::Spikes:
                    kill(entity);
                    break;
                case CollisionShape::Player:
                    
                    if (manifold->normal.x != 0)
                    {
                        //DPRINT("Player", "Collision");
                        crate.velocity.x += manifold->normal.x * 80.f; //TODO const this impulse
                        tx.move(manifold->normal * manifold->penetration);
                    }
                    break;
                }

            }
        }
    }
}

void CrateSystem::kill(xy::Entity entity)
{
    auto& crate = entity.getComponent<Crate>();
    crate.state = Crate::Dead;
    crate.stateTime = Crate::DeadTime;

    auto& tx = entity.getComponent<xy::Transform>();
    auto scale = tx.getScale();
    scale.x = 0.f;
    tx.setScale(scale);
}