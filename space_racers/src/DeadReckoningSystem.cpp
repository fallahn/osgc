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

#include "DeadReckoningSystem.hpp"
#include "NetActor.hpp"
#include "ActorIDs.hpp"
#include "CollisionObject.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, float t)
    {
        return (a * t) + (b * (1.f - t));
    }
}

DeadReckoningSystem::DeadReckoningSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(DeadReckoningSystem))
{
    requireComponent<DeadReckon>();
    requireComponent<xy::Transform>();
    requireComponent<NetActor>();
}

//public
void DeadReckoningSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& dr = entity.getComponent<DeadReckon>();
        
        //use the update to set the position, and the perceived
        //latency to move the entity forward
        if (dr.hasUpdate)
        {
            dr.hasUpdate = false;
            auto latency = dr.update.timestamp - dr.prevTimestamp;
            float delta = static_cast<float>(latency) / 1000.f;
            dr.prevTimestamp = dr.update.timestamp;

            auto& tx = entity.getComponent<xy::Transform>();
            tx.setPosition(dr.update.x, dr.update.y);

            sf::Vector2f newVel(dr.update.velX, dr.update.velY);
            tx.move(newVel * delta);
            //collision(entity);

            //tx.setRotation(dr.update.rotation); //we'll use normal interpolation for rotation

            dr.targetTime = delta;
            dr.currentTime = 0.f;
            dr.currVelocity = dr.targetVelocity;
            dr.targetVelocity = newVel;
        }
        else
        {
            //use the current known velocity to move forward

            dr.currentTime = std::min(dr.targetTime, dr.currentTime + dt);
            sf::Vector2f vel = lerp(dr.currVelocity, dr.targetVelocity, dr.currentTime / dr.targetTime);

            auto& tx = entity.getComponent<xy::Transform>();
            tx.move(vel * dt);
            //collision(entity);
        }
    }
}

//private
void DeadReckoningSystem::collision(xy::Entity entity)
{
    //some crude collision detection just to update the predicted velocity
    auto bounds = entity.getComponent<xy::BroadphaseComponent>().getArea();
    auto& tx = entity.getComponent<xy::Transform>();
    bounds = tx.getTransform().transformRect(bounds);

    //if we split the types we can do cheaper circle collision on roids
    const auto& actor = entity.getComponent<NetActor>();
    if (actor.actorID == ActorID::Roid)
    {
        auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds, CollisionFlags::Asteroid);
        //for asteroids the origin is centre so we can assume it as radius
        float radius = tx.getOrigin().x * tx.getScale().x;

        for (auto e : nearby)
        {
            if (e != entity)
            {
                const auto& otherTx = e.getComponent<xy::Transform>();
                float otherRadius = otherTx.getOrigin().x * otherTx.getScale().x;

                float minDist = radius + otherRadius;
                float minDist2 = minDist * minDist;

                auto diff = tx.getPosition() - otherTx.getPosition();
                float len2 = xy::Util::Vector::lengthSquared(diff);

                if (len2 < minDist2)
                {
                    float len = std::sqrt(len2);
                    float penetration = minDist - len;
                    sf::Vector2f normal = diff / len;

                    tx.move(normal * penetration);

                    auto& dr = entity.getComponent<DeadReckon>();
                    sf::Vector2f vel = xy::Util::Vector::reflect({ dr.update.velX, dr.update.velY }, diff);
                    dr.update.velX = vel.x;
                    dr.update.velY = vel.y;
                    dr.currVelocity = dr.targetVelocity;
                    dr.targetVelocity = vel;
                }
            }
        }
    }
}