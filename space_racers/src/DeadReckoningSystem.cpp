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

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Vector.hpp>

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
            tx.move(sf::Vector2f(dr.update.velX, dr.update.velY) * delta);
            //tx.setRotation(dr.update.rotation); //we'll use normal interpolation for rotation
        }
        else
        {
            //use the current known velocity to move forward
            auto& tx = entity.getComponent<xy::Transform>();
            tx.move(sf::Vector2f(dr.update.velX, dr.update.velY) * dt);
        }
    }
}

//private
void DeadReckoningSystem::collision(xy::Entity entity)
{
    //some crude collision detection just to update the predicted velocity
    auto bounds = entity.getComponent<xy::BroadphaseComponent>().getArea();
    const auto& tx = entity.getComponent<xy::Transform>();
    bounds = tx.getTransform().transformRect(bounds);

    //if we split the types we can do cheaper circle collision on roids
    const auto& actor = entity.getComponent<NetActor>();
    if (actor.actorID == ActorID::Roid)
    {
        auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds);
        //for asteroids the origin is centre so we can assume it as radius
        float radius = tx.getOrigin().x * tx.getScale().x;

        for (auto e : nearby)
        {
            const auto& otherTx = e.getComponent<xy::Transform>();
            float otherRadius = otherTx.getOrigin().x * otherTx.getScale().x;

            float otherLen2 = radius + otherRadius;
            otherLen2 *= otherLen2;

            auto diff = otherTx.getPosition() - tx.getPosition();
            float len2 = xy::Util::Vector::lengthSquared(diff);

            if (len2 < otherLen2)
            {
                float len = std::sqrt(len2);
                diff /= len;

                auto& dr = entity.getComponent<DeadReckon>();
                sf::Vector2f vel = xy::Util::Vector::reflect({ dr.update.velX, dr.update.velY }, diff);
                dr.update.velX = vel.x;
                dr.update.velY = vel.y;
            }
        }
    }
}