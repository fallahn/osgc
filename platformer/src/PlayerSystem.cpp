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
    //TODO apply gravity
    applyVelocity(entity, dt);

    doCollision(entity, dt);
}

void PlayerSystem::processRunning(xy::Entity entity, float dt)
{
    doInput(entity);
    applyVelocity(entity, dt);

    doCollision(entity, dt);
}

void PlayerSystem::processDying(xy::Entity entity, float dt)
{

}

void PlayerSystem::doCollision(xy::Entity entity, float)
{
    auto& player = entity.getComponent<Player>();
    auto& tx = entity.getComponent<xy::Transform>();

    //broad phase
    auto queryArea = tx.getTransform().transformRect(entity.getComponent<xy::BroadphaseComponent>().getArea());
    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryArea, CollisionShape::Water | CollisionShape::Solid);

    for (auto other : nearby)
    {
        if (other != entity)
        {
            auto otherBounds = other.getComponent<xy::Transform>().getTransform().transformRect(other.getComponent<xy::BroadphaseComponent>().getArea());
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
    const auto& collisionBody = entity.getComponent<CollisionBody>();
    auto& player = entity.getComponent<Player>();
    player.footSense = false;
    
    //in theory we need to test all the shapes in the other entity, not its global
    //AABB, but for now assume we're only colliding with platforms
    for (auto i = 0u; i < collisionBody.shapeCount; ++i)
    {
        auto bounds = tx.getTransform().transformRect(collisionBody.shapes[i].aabb);
        if (auto manifold = intersectsAABB(bounds, otherBounds); manifold)
        {
            if (collisionBody.shapes[i].type == CollisionShape::Player)
            {
                tx.move(manifold->normal * manifold->penetration);
                player.velocity = xy::Util::Vector::reflect(player.velocity, manifold->normal) * 0.7f;
            }
            else if (collisionBody.shapes[i].type == CollisionShape::Sensor)
            {
                player.footSense = true;
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
}

void PlayerSystem::applyVelocity(xy::Entity entity, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& player = entity.getComponent<Player>();

    tx.move(player.velocity * dt);
    player.velocity *= Player::Drag;
}