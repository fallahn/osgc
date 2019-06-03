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
#include "AsteroidSystem.hpp"
#include "CollisionObject.hpp"
#include "VehicleSystem.hpp"

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

            tx.setRotation(dr.update.rotation);

            if (entity.getComponent<NetActor>().actorID == ActorID::Roid)
            {
                entity.getComponent<Asteroid>().setVelocity(newVel);
            }
            //else if vehicle set vel/rotVel
            //TODO ideally we want to sync entire state? much like
            //reconciliation only applying the last known input
            else
            {
                auto& vehicle = entity.getComponent<Vehicle>();
                vehicle.velocity = newVel;
                
                dr.lastExtrapolatedTimestamp = dr.update.timestamp + (dr.update.timestamp - dr.prevTimestamp);

                //apply input - TODO pop this in a func so we don't repeat code below
                Input input;
                input.flags = dr.update.lastInput;
                input.timestamp = dr.lastExtrapolatedTimestamp;
                //shame we don't have analogue multipliers...
                //we'll either have to see what bw we have or split roid/vehicle updates to different packets

                vehicle.history[vehicle.currentInput] = input;
                vehicle.currentInput = (vehicle.currentInput + 1) % vehicle.history.size();
            }
        }
        
        else
        {
            //else if vehicle apply current input to vehicle system
            //while updating timestamp

            if (entity.getComponent<NetActor>().actorID != ActorID::Roid)
            {
                dr.lastExtrapolatedTimestamp += static_cast<std::int32_t>(dt * 1000000.f);

                Input input;
                input.flags = dr.update.lastInput;
                input.timestamp = dr.lastExtrapolatedTimestamp;

                auto& vehicle = entity.getComponent<Vehicle>();
                vehicle.history[vehicle.currentInput] = input;
                vehicle.currentInput = (vehicle.currentInput + 1) % vehicle.history.size();


                //use the current known velocity to move forward
                /*sf::Vector2f vel = { dr.update.velX, dr.update.velY };
                auto& tx = entity.getComponent<xy::Transform>();
                tx.move(vel * dt);*/

            }
        }
    }
}

//private