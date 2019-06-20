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

#include "CameraTarget.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float SmoothTime = 0.13f;
    const float MaxSpeed = 500.f;

    //smoothing based on game programming gems 4, chapter 1.10
    float smoothMotion(float from, float to, float& vel, float dt)
    {
        float omega = 2.f / SmoothTime;
        float x = omega * dt;
        float exp = 1.f / (1.f + x + 0.48f * x * x + 0.235f * x * x * x);
        float change = from - to;

        float maxChange = MaxSpeed * SmoothTime;
        change = std::min(std::max(-maxChange, change), maxChange);

        float temp = (vel + omega * change) * dt;
        vel = (vel - omega * temp) * exp;
        return to + (change + temp) * exp;
    }

    sf::Vector2f smoothMotion(sf::Vector2f from, sf::Vector2f to, sf::Vector2f& vel, float dt)
    {
        return { smoothMotion(from.x, to.x, vel.x, dt), smoothMotion(from.y, to.y, vel.y, dt) };
    }
}

CameraTargetSystem::CameraTargetSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(CameraTargetSystem))
{
    requireComponent<CameraTarget>();
    requireComponent<xy::Transform>();
}

//public
void CameraTargetSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto& camera = entity.getComponent<CameraTarget>();
        
        if (camera.target != camera.lastTarget)
        {
            camera.lastTarget = camera.target;
            camera.velocity = {};
        }

        if (camera.target.isValid())
        {
            auto destPosition = camera.target.getComponent<xy::Transform>().getPosition();
            auto txPosition = tx.getPosition();

            tx.setPosition(smoothMotion(txPosition, destPosition, camera.velocity, dt));
        }
    }
}