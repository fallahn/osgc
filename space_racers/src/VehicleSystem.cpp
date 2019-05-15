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
#include "ServerPackets.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/util/Const.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/gui/Gui.hpp>

#include <cmath>

VehicleSystem::VehicleSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(VehicleSystem))
{
    requireComponent<Vehicle>();
    requireComponent<xy::Transform>();

#ifdef FAST_SIN
    ft::init();
#endif
}

//public
void VehicleSystem::process(float)
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

            auto delta = getDelta(vehicle.history, vehicle.lastUpdatedInput);
            applyInput(entity, delta);

            //TODO collision

            vehicle.lastUpdatedInput = (vehicle.lastUpdatedInput + 1) % vehicle.history.size();
        }
    }
}

void VehicleSystem::reconcile(const ClientUpdate& update, xy::Entity entity)
{
    //sync state
    auto& vehicle = entity.getComponent<Vehicle>();
    vehicle.velocity = { update.velX, update.velY };
    vehicle.anglularVelocity = update.velRot;

    auto& tx = entity.getComponent<xy::Transform>();
    tx.setPosition(update.x, update.y);
    tx.setRotation(update.rotation);

    //redo prediction from last known server input
    auto ip = std::find_if(vehicle.history.rbegin(), vehicle.history.rend(),
        [&update](const Input & input)
        {
            return update.clientTimestamp == input.timestamp;
        });

    if (ip != vehicle.history.rend())
    {
        std::size_t idx = std::distance(vehicle.history.begin(), ip.base()) - 1;
        auto end = (vehicle.currentInput + vehicle.history.size() - 1) % vehicle.history.size();

        while (idx != end) //currentInput points to the next free slot in history
        {
            float delta = getDelta(vehicle.history, idx);
            applyInput(entity, delta);            

            idx = (idx + 1) % vehicle.history.size();
        }
    }
}

//private
void VehicleSystem::processInput(xy::Entity entity)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    auto input = vehicle.history[vehicle.lastUpdatedInput];

    //acceleration
    auto acceleration = 0.f;
    if (input.flags & InputFlag::Accelerate)
    {
        acceleration += vehicle.settings.acceleration * input.accelerationMultiplier;
    }

    //rotation
    const auto& tx = entity.getComponent<xy::Transform>();
    auto rotation = tx.getRotation() * xy::Util::Const::degToRad;

#ifdef FAST_SIN
    //TODO if the table is pre-baked it would also be more consistent across platforms
    vehicle.velocity += sf::Vector2f(ft::cos(rotation), ft::sin(rotation)) * acceleration;
#else
    vehicle.velocity += sf::Vector2f(std::cos(rotation), std::sin(rotation)) * acceleration;
#endif //FAST_SIN

    if (input.flags & InputFlag::Brake)
    {
        //acceleration -= vehicle.settings.acceleration * vehicle.settings.brakeStrength;
        vehicle.velocity *= vehicle.settings.brakeStrength;
    }

    if (input.flags & InputFlag::Left)
    {
        vehicle.anglularVelocity -= vehicle.settings.turnSpeed * input.steeringMultiplier;
    }

    if (input.flags & InputFlag::Right)
    {
        vehicle.anglularVelocity += vehicle.settings.turnSpeed * input.steeringMultiplier;
    }
}

void VehicleSystem::applyInput(xy::Entity entity, float dt)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    auto& tx = entity.getComponent<xy::Transform>();

    tx.move(vehicle.velocity * dt);
    vehicle.velocity *= vehicle.settings.drag;

    //rotate more slowly at lower speeds
    float rotationMultiplier = std::min(1.f, xy::Util::Vector::lengthSquared(vehicle.velocity) / (vehicle.settings.maxSpeedSqr() / 8.f));
    rotationMultiplier = 0.03f + (rotationMultiplier * 0.97f);

    tx.rotate(((xy::Util::Const::radToDeg * vehicle.anglularVelocity) * rotationMultiplier) * dt);
    vehicle.anglularVelocity *= vehicle.settings.angularDrag;
}

float VehicleSystem::getDelta(const History& history, std::size_t idx)
{
    auto prevInput = (idx + history.size() - 1) % history.size();

    //skip any inputs in the wrong order
    /*while (history[prevInput].timestamp > history[idx].timestamp)
    {
        prevInput = (prevInput + history.size() - 1) % history.size();
    }*/

    auto delta = history[idx].timestamp - history[prevInput].timestamp;

    return static_cast<float>(delta) / 1000000.f;
}