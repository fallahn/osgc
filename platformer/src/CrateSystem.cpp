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
#include <xyginext/ecs/components/ParticleEmitter.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Vector.hpp>
#include <xyginext/detail/Operators.hpp>

namespace
{
    const float Gravity = 5999.f;
    const float TerminalVelocity = 12000.f;

    //TODO as with dropping items, if this is made more generic
    //move elsewhere - possible to game consts?
    const sf::Vector2f CarryOffset(16.f, -10.f);
}

CrateSystem::CrateSystem(xy::MessageBus& mb, SharedData& sd)
    : xy::System    (mb, typeid(CrateSystem)),
    m_sharedData    (sd)
{
    requireComponent<Crate>();
    requireComponent<xy::BroadphaseComponent>();
    requireComponent<xy::Transform>();
    requireComponent<xy::ParticleEmitter>();
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
                crateTx.setPosition(position + (crateTx.getOrigin() * tx.getScale()));
                crateTx.setScale(tx.getScale().y, tx.getScale().y); //not a mistake, don't want negative x vals
                tx.removeChild(crateTx);

                player.carriedItem = {};
            }
            else
            {
                //scan area for crates and pick up
                auto& tx = playerEnt.getComponent<xy::Transform>();
                auto position = tx.getWorldPosition();
                position += tx.getOrigin() * tx.getScale();
                auto offset = CarryOffset;
                offset = offset * tx.getScale();

                position += offset;

                //TODO constify
                sf::FloatRect queryArea(position.x - 32.f, position.y - 32.f, 64.f, 64.f);
                auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryArea, CollisionShape::Crate);
                for (auto crate : nearby)
                {
                    auto& crateTx = crate.getComponent<xy::Transform>();
                    auto bounds = crate.getComponent<xy::BroadphaseComponent>().getArea();
                    bounds = crateTx.getWorldTransform().transformRect(bounds);
                    auto origin = crateTx.getOrigin() * tx.getScale().y;
                    bounds.left += origin.x;
                    bounds.top += origin.y;

                    if (bounds.contains(position))
                    {
                        if (crateTx.getDepth() > 0) //assumes parented to a platform... 
                        {
                            detachPlatform(crate);
                        }

                        //pick up crate
                        crateTx.setPosition(22.f, 6.f); //no idea what relation this has to offset - subtracted origin?
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
        if (data.type == StarEvent::HitItem &&
            data.entityHit.getComponent<xy::BroadphaseComponent>().getFilterFlags() & CollisionShape::Crate)
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
            //DPRINT("State", "Falling");
            break;
        case Crate::Idle:
            updateIdle(entity, dt);
            //DPRINT("State", "Idle");
            break;
        case Crate::Dead:
            updateDead(entity, dt);
            //DPRINT("State", "Dead");
            break;
        }
    }
}

//private
void CrateSystem::updateFalling(xy::Entity entity, float dt)
{
    auto& crate = entity.getComponent<Crate>();
    crate.velocity.y = std::min(crate.velocity.y + Gravity * dt, TerminalVelocity);

    crate.platform = {};

    applyVelocity(entity, dt);
    doCollision(entity);

    //check which shapes have collision
    if (crate.state != Crate::Dead) //collision may have killed it
    {
        auto flags = entity.getComponent<CollisionBody>().collisionFlags;
        if (flags & (CollisionShape::Crate/* | CollisionShape::Foot*/))
        {
            //touched the ground
            crate.velocity = {};
            crate.state = Crate::Idle;

            auto& tx = entity.getComponent<xy::Transform>();

            //parent to moving platform if landed on one
            if (crate.platform.isValid()
                && (flags & CollisionShape::Foot))
            {
                auto relPos = tx.getPosition() - crate.platform.getComponent<xy::Transform>().getPosition();
                tx.setPosition(relPos);
                crate.platform.getComponent<xy::Transform>().addChild(tx);
            }

            //raise a message
            auto* msg = postMessage<CrateEvent>(MessageID::CrateMessage);
            msg->type = CrateEvent::Landed;
            msg->position = tx.getWorldPosition() + (tx.getOrigin() * tx.getScale());
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

            //unparent from moving platforms
            if (crate.platform.isValid())
            {
                detachPlatform(entity);
            }
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

        auto* msg = postMessage<CrateEvent>(MessageID::CrateMessage);
        msg->type = CrateEvent::Spawned;
        msg->position = tx.getPosition();
    }
}

void CrateSystem::detachPlatform(xy::Entity entity)
{

    auto& tx = entity.getComponent<xy::Transform>();
    if (tx.getDepth() > 0)
    {
        auto& crate = entity.getComponent<Crate>();
        auto worldPos = tx.getPosition() + crate.platform.getComponent<xy::Transform>().getPosition();
        tx.setPosition(worldPos);
        crate.platform.getComponent<xy::Transform>().removeChild(tx);

        crate.platform = {};
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
    //DPRINT("Nearby", std::to_string(nearby.size()));
}

void CrateSystem::resolveCollision(xy::Entity entity, xy::Entity other, sf::FloatRect otherBounds)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& collisionBody = entity.getComponent<CollisionBody>();
    auto& crate = entity.getComponent<Crate>();

    for (auto i = 0u; i < collisionBody.shapeCount; ++i)
    {
        auto bounds = collisionBody.shapes[i].aabb;
        bounds = boundsToWorldSpace(bounds, tx);

        if (auto manifold = intersectsAABB(bounds, otherBounds); manifold)
        {
            auto& otherBody = other.getComponent<CollisionBody>();

            //flag crate body as having a collision on this shape
            if (otherBody.shapes[0].type & (CollisionShape::Solid | CollisionShape::MPlat))
            {
                collisionBody.collisionFlags |= collisionBody.shapes[i].type;
                //DPRINT("Type", std::to_string(collisionBody.shapes[i].type));
            }

            if (collisionBody.shapes[i].type == CollisionShape::Crate)
            {
                switch (otherBody.shapes[0].type)
                {
                default: break;
                case CollisionShape::MPlat:
                    //set player platform if not set
                    if (!crate.platform.isValid()
                        && manifold->normal.y < 0)
                    {
                       crate.platform = other;
                    } //fall through to solid physics
                case CollisionShape::Crate:
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
                case CollisionShape::Enemy:
                    if (manifold->normal.y != 0)
                    {
                        if (manifold->normal.y < 0)
                        {
                            //crate on top, so kill enemy
                            auto* msg = postMessage<CrateEvent>(MessageID::CrateMessage);
                            msg->type = CrateEvent::KilledEnemy;
                            msg->entityHit = other;
                            msg->position = tx.getPosition();
                        }
                        kill(entity);
                    }
                    else
                    {
                        //if penetration above a certain threshold, kill the enemy/crate
                        if (manifold->penetration > 48)
                        {
                            auto* msg = postMessage<CrateEvent>(MessageID::CrateMessage);
                            msg->type = CrateEvent::KilledEnemy;
                            msg->entityHit = other;
                            msg->position = tx.getPosition();

                            kill(entity);
                        }
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

    entity.getComponent<xy::ParticleEmitter>().start();

    auto* msg = postMessage<CrateEvent>(MessageID::CrateMessage);
    msg->type = CrateEvent::Destroyed;
    msg->position = tx.getPosition();
}