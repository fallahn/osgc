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

#include "VehicleSystem.hpp"
#include "InputBinding.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/util/Const.hpp>
#include <xyginext/util/Vector.hpp>

#include <cmath>

VehicleSystem::VehicleSystem(xy::MessageBus& mb)
    :xy::System(mb, typeid(VehicleSystem))
{
    requireComponent<Vehicle>();
    requireComponent<xy::Transform>();
}

//public
void VehicleSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& vehicle = entity.getComponent<Vehicle>();

        //process outstanding input (current input actually points to next empty slot)
        std::size_t idx = (vehicle.currentInput + vehicle.history.size() - 1 ) % vehicle.history.size();

        while (vehicle.lastUpdatedInput != idx)
        {
            processInput(entity);
            applyInput(entity, dt);

            //TODO collision

            vehicle.lastUpdatedInput = (vehicle.lastUpdatedInput + 1) % vehicle.history.size();
        }

        DPRINT("Acceleration", std::to_string(xy::Util::Vector::length(vehicle.velocity)));
    }
}

//private
void VehicleSystem::processInput(xy::Entity entity)
{
    //TODO if we're spending a long time here
    //optimise sin/cos with lookup table
    //if the table is pre-baked it would also be more consistent across platforms
    auto& vehicle = entity.getComponent<Vehicle>();
    auto input = vehicle.history[vehicle.lastUpdatedInput];

    //acceleration
    auto acceleration = 0.f;
    if (input.flags & InputFlag::Accelerate)
    {
        acceleration += vehicle.acceleration;
    }

    if (input.flags & InputFlag::Brake)
    {
        acceleration -= vehicle.acceleration * Vehicle::brakeStrength;
    }

    //rotation
    const auto& tx = entity.getComponent<xy::Transform>();
    auto rotation = -tx.getRotation() * xy::Util::Const::degToRad;
    vehicle.velocity += sf::Vector2f(std::sin(rotation), std::cos(rotation)) * acceleration;

    if (input.flags & InputFlag::Left)
    {
        vehicle.anglularVelocity -= vehicle.turnSpeed;
    }

    if (input.flags & InputFlag::Right)
    {
        vehicle.anglularVelocity += vehicle.turnSpeed;
    }
}

void VehicleSystem::applyInput(xy::Entity entity, float dt)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    auto& tx = entity.getComponent<xy::Transform>();

    tx.move(vehicle.velocity * dt);
    vehicle.velocity *= vehicle.drag;

    //rotate more slowly at lower speeds
    float rotationMultiplier = xy::Util::Vector::lengthSquared(vehicle.velocity) / Vehicle::maxSpeedSqr();
    rotationMultiplier = 0.1f + (rotationMultiplier * 0.9f);

    tx.rotate(((xy::Util::Const::radToDeg * vehicle.anglularVelocity) * rotationMultiplier) * dt);
    vehicle.anglularVelocity *= vehicle.angularDrag;
}