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

#define FAST_SIN
#ifdef FAST_SIN
#include "FastTrig.hpp"
#endif //FAST_SIN

#include "VehicleSystem.hpp"
#include "InputBinding.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/util/Const.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/gui/Gui.hpp>

#include <cmath>

namespace
{
#ifdef XY_DEBUG
    namespace Test
    {
        //test values for imgui output
        float angularDrag = 0.9f;
        float turnSpeed = 0.76f; //rads per second

        float drag = 0.93f; //multiplier
        float acceleration = 84.f; //units per second

        float maxSpeed = 1080.f;
    }
#endif //XY_DEBUG
}

VehicleSystem::VehicleSystem(xy::MessageBus& mb)
    :xy::System(mb, typeid(VehicleSystem))
{
    requireComponent<Vehicle>();
    requireComponent<xy::Transform>();

#ifdef XY_DEBUG
    registerWindow(
        []()
        {
            xy::Nim::begin("Vehicle Properties");
            xy::Nim::slider("Drag", Test::drag, 0.f, 0.995f);
            xy::Nim::slider("MaxSpeed", Test::maxSpeed, 400.f, 1200.f);
            Test::acceleration = Test::maxSpeed / (Test::drag / (1.f - Test::drag));
            xy::Nim::text("Acc. " + std::to_string(Test::acceleration));

            xy::Nim::separator();
            xy::Nim::slider("Angular Drag", Test::angularDrag, 0.f, 0.995f);
            xy::Nim::slider("Turn Speed", Test::turnSpeed, 0.1f, 1.5f);

            xy::Nim::end();
        });
#endif //XY_DEBUG


#ifdef FAST_SIN
    ft::init();
#endif
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
    }
}

//private
void VehicleSystem::processInput(xy::Entity entity)
{
    //TODO if the table is pre-baked it would also be more consistent across platforms
    auto& vehicle = entity.getComponent<Vehicle>();
    auto input = vehicle.history[vehicle.lastUpdatedInput];

    //acceleration
    auto acceleration = 0.f;
    if (input.flags & InputFlag::Accelerate)
    {
        acceleration += Test::acceleration;
        //acceleration += Vehicle::acceleration;
    }

    if (input.flags & InputFlag::Brake)
    {
        acceleration -= Test::acceleration * Vehicle::brakeStrength;
        //acceleration -= Vehicle::acceleration * Vehicle::brakeStrength;
    }

    //rotation
    const auto& tx = entity.getComponent<xy::Transform>();
    auto rotation = -tx.getRotation() * xy::Util::Const::degToRad;

#ifdef FAST_SIN
    vehicle.velocity += sf::Vector2f(ft::sin(rotation), ft::cos(rotation)) * acceleration;
#else
    vehicle.velocity += sf::Vector2f(std::sin(rotation), std::cos(rotation)) * acceleration;
#endif //FAST_SIN

    if (input.flags & InputFlag::Left)
    {
        vehicle.anglularVelocity -= Test::turnSpeed;
        //vehicle.anglularVelocity -= Vehicle::turnSpeed;
    }

    if (input.flags & InputFlag::Right)
    {
        vehicle.anglularVelocity += Test::turnSpeed;
        //vehicle.anglularVelocity += Vehicle::turnSpeed;
    }
}

void VehicleSystem::applyInput(xy::Entity entity, float dt)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    auto& tx = entity.getComponent<xy::Transform>();

    tx.move(vehicle.velocity * dt);
    vehicle.velocity *= Test::drag;
    //vehicle.velocity *= Vehicle::drag;

    //rotate more slowly at lower speeds
    float rotationMultiplier = xy::Util::Vector::lengthSquared(vehicle.velocity) / (Test::maxSpeed * Test::maxSpeed);
    //float rotationMultiplier = xy::Util::Vector::lengthSquared(vehicle.velocity) / Vehicle::maxSpeedSqr();
    rotationMultiplier = 0.1f + (rotationMultiplier * 0.9f);

    tx.rotate(((xy::Util::Const::radToDeg * vehicle.anglularVelocity) * rotationMultiplier) * dt);
    vehicle.anglularVelocity *= Test::angularDrag;
    //vehicle.anglularVelocity *= Vehicle::angularDrag;
}