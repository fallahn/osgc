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

#include "AIDriverSystem.hpp"
#include "VehicleSystem.hpp"
#include "WayPoint.hpp"
#include "InputBinding.hpp"
#include "InverseRotationSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>

namespace
{
    //calculates cross product https://en.wikipedia.org/wiki/Cross_product
    float lineSide(sf::Vector2f position, sf::Vector2f direction, sf::Vector2f point) 
    {
        return ((direction.x - position.x) * (point.y - position.y) - (direction.y - position.y) * (point.x - position.x));
    }

    const std::array<float, 3u> skills = { 0.9f, 0.7f, 0.4f };
}

AIDriverSystem::AIDriverSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(AIDriverSystem))
{
    requireComponent<AIDriver>();
    requireComponent<Vehicle>();
}

//public
void AIDriverSystem::process(float dt)
{
    auto frameTime = static_cast<std::int32_t>(dt * 1000000.f);

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& ai = entity.getComponent<AIDriver>();
        ai.timestamp += frameTime;

        //get target
        auto& vehicle = entity.getComponent<Vehicle>();
        if (vehicle.currentWaypoint != ai.currentWaypoint)
        {
            const auto& waypoint = vehicle.currentWaypoint.getComponent<WayPoint>();
            ai.target = vehicle.currentWaypoint.getComponent<xy::Transform>().getPosition() +  (waypoint.nextPoint * waypoint.nextDistance);
            ai.currentWaypoint = vehicle.currentWaypoint;
        }

        //get forward vector
        const auto& tx = entity.getComponent<xy::Transform>();

        Transform tf;
        tf.setRotation(tx.getRotation());
        sf::Vector2f forwardVec = tf.transformPoint(1.f, 0.f);

        //adjust steering based on which side of forward ray target is on
        static const float threshold = 80.f;
        Input input;
        float side = lineSide(tx.getPosition(), tx.getPosition() + forwardVec, ai.target);
        float thresholdAmount = threshold + (10.f * skills[ai.skill]);
        if (side > thresholdAmount)
        {
            input.flags |= InputFlag::Right;
        }
        else if (side < -thresholdAmount)
        {
            input.flags |= InputFlag::Left;
        }

        //adjust acceleration multiplier based on distance to target (ie slow down when nearer)
        input.flags |= InputFlag::Accelerate;
        if (ai.currentWaypoint.isValid())
        {
            const auto& waypoint = ai.currentWaypoint.getComponent<WayPoint>();

            float distance = vehicle.totalDistance - vehicle.waypointDistance;
            float waypointDistance = waypoint.nextDistance;

            if (distance > waypointDistance * 0.8f)
            {
                distance = waypointDistance * 0.2f;
                distance = 1.f - (distance / waypointDistance);
            }
            else
            {
                distance = 1.f;
            }
            //TODO modify the  skill amount depending on how near the front / back
            //the AI is in relation to the pack
            //TODO modify this if another car is close in front
            input.accelerationMultiplier = skills[ai.skill] + ((1.f - skills[ai.skill]) * distance);

            //look to see how sharp a turn is coming up and slow down if needed
            if (vehicle.waypointDistance > 0)
            {
                float sharpness = 1.f - (xy::Util::Vector::dot(forwardVec, ai.target - tx.getPosition()) / vehicle.waypointDistance);
                sharpness = 0.8f + (0.2f * sharpness);
                input.accelerationMultiplier *= sharpness;
            }
        }

        //TODO sweep for collidable objects and steer away from? solids or space
        //TODO faux acceleration by modifying input? Could use this for differing types of AI

        //update timestamp and apply input to vehicle
        input.timestamp = ai.timestamp;

        vehicle.history[vehicle.currentInput] = input;
        vehicle.currentInput = (vehicle.currentInput + 1) % vehicle.history.size();
    }
}