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

#include "Player.hpp"
#include "InputBinding.hpp"
#include "Collision.hpp"

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    sf::FloatRect boundsToWorldspace(sf::FloatRect bounds, const xy::Transform& tx)
    {
        auto scale = tx.getScale();
        bounds.left *= scale.x;
        bounds.width *= scale.x;
        bounds.top *= scale.y;
        bounds.height *= scale.y;
        bounds.left += tx.getPosition().x;
        bounds.top += tx.getPosition().y;

        return bounds;
    }

    const float Gravity = 5999.9f;
    const float JumpImpulse = 2200.f;
}

PlayerSystem::PlayerSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(PlayerSystem))
{
    requireComponent<xy::Transform>();
    requireComponent<xy::BroadphaseComponent>();
    requireComponent<CollisionBody>();
    requireComponent<Player>();
}

//public
void PlayerSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& player = entity.getComponent<Player>();
        switch (player.state)
        {
        default: break;
        case Player::Falling:
            processFalling(entity, dt);
            break;
        case Player::Running:
            processRunning(entity, dt);
            break;
        case Player::Dying:
            processDying(entity, dt);
            break;
        }
    }
}

//private
void PlayerSystem::processFalling(xy::Entity entity, float dt)
{
    doInput(entity);
    //apply gravity
    entity.getComponent<Player>().velocity.y += Gravity * dt;

    applyVelocity(entity, dt);
    doCollision(entity, dt);

    //check collision flags and change state if necessary
    if (entity.getComponent<CollisionBody>().collisionFlags & (CollisionShape::Player | CollisionShape::Sensor))
    {
        entity.getComponent<Player>().state = Player::Running;
    }
}

void PlayerSystem::processRunning(xy::Entity entity, float dt)
{
    doInput(entity);
    applyVelocity(entity, dt);

    doCollision(entity, dt);

    if ((entity.getComponent<CollisionBody>().collisionFlags & CollisionShape::Sensor) == 0)
    {
        entity.getComponent<Player>().state = Player::Falling;
    }
}

void PlayerSystem::processDying(xy::Entity entity, float dt)
{

}

void PlayerSystem::doCollision(xy::Entity entity, float)
{
    auto& player = entity.getComponent<Player>();
    auto& tx = entity.getComponent<xy::Transform>();
    entity.getComponent<CollisionBody>().collisionFlags = 0;

    //broad phase
    auto area = entity.getComponent<xy::BroadphaseComponent>().getArea();

    auto queryArea = boundsToWorldspace(area, tx);
    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryArea, CollisionShape::Water | CollisionShape::Solid);

    for (auto other : nearby)
    {
        if (other != entity)
        {
            other.getComponent<CollisionBody>().collisionFlags = 0; //TODO remove this

            auto otherBounds = boundsToWorldspace(other.getComponent<xy::BroadphaseComponent>().getArea(), other.getComponent<xy::Transform>());

            if (otherBounds.intersects(queryArea))
            {
                resolveCollision(entity, other, otherBounds);
            }
        }
    }
}

void PlayerSystem::resolveCollision(xy::Entity entity, xy::Entity other, sf::FloatRect otherBounds)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& collisionBody = entity.getComponent<CollisionBody>();
    auto& player = entity.getComponent<Player>();
    
    //in theory we need to test all the shapes in the other entity, not its global
    //AABB, but for now assume we're only colliding with platforms
    for (auto i = 0u; i < collisionBody.shapeCount; ++i)
    {
        auto bounds = collisionBody.shapes[i].aabb;
        bounds = boundsToWorldspace(bounds, tx);

        if (auto manifold = intersectsAABB(bounds, otherBounds); manifold)
        {
            auto& otherBody = other.getComponent<CollisionBody>();
            collisionBody.collisionFlags |= collisionBody.shapes[i].type;
            otherBody.collisionFlags |= collisionBody.shapes[i].type; //TODO remove this, we shouldn't be mutating the other object's state - just using for debug draw atm

            if (collisionBody.shapes[i].type == CollisionShape::Player)
            {
                tx.move(manifold->normal * manifold->penetration);

                if (manifold->normal.y < 0)
                {
                    player.velocity = {};
                }
                else
                {
                    player.velocity = xy::Util::Vector::reflect(player.velocity, manifold->normal) * 0.17f;
                }
            }
            else if (collisionBody.shapes[i].type == CollisionShape::Sensor)
            {

            }
        }
    }
}

void PlayerSystem::doInput(xy::Entity entity/*, float dt*/)
{
    auto& player = entity.getComponent<Player>();
    if (player.input & InputFlag::Left)
    {
        player.velocity.x -= Player::Acceleration * player.accelerationMultiplier;
    }

    if (player.input & InputFlag::Right)
    {
        player.velocity.x += Player::Acceleration * player.accelerationMultiplier;
    }

    if ((player.input & InputFlag::Jump)
        && (player.prevInput & InputFlag::Jump) == 0
        && player.state != Player::Falling)
    {
        player.velocity.y -= JumpImpulse;
        player.state = Player::Falling;
    }

    if (player.input & InputFlag::Shoot)
    {
        //player.velocity.y += Player::Acceleration;
    }

    player.prevInput = player.input;
}

void PlayerSystem::applyVelocity(xy::Entity entity, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& player = entity.getComponent<Player>();

    tx.move(player.velocity * dt);
    player.velocity *= Player::Drag;
}