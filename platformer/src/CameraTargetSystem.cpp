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
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Wavetable.hpp>

namespace
{
    const float SmoothTime = 0.13f; //this is quite sensitive - anything more than .15 is pretty sloppy
    const float MaxSpeed = 8000.f;

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
    : xy::System(mb, typeid(CameraTargetSystem)),
    m_shakeIndex(0)
{
    requireComponent<CameraTarget>();
    requireComponent<xy::Transform>();

    m_shakeTable = xy::Util::Wavetable::sine(8.f, 5.f);
}

//public
void CameraTargetSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto& camera = entity.getComponent<CameraTarget>();
        
        if (camera.target.isValid())
        {
            //smooth the target points should the target change
            if (camera.target != camera.lastTarget)
            {
                if (camera.lastTarget.isValid())
                {
                    auto targetPosition = camera.target.getComponent<xy::Transform>().getWorldPosition();

                    auto diff = targetPosition - camera.lastTarget.getComponent<xy::Transform>().getWorldPosition();
                    float len2 = xy::Util::Vector::lengthSquared(diff);

                    if (len2 > 25)
                    {
                        camera.targetPosition = smoothMotion(camera.targetPosition, targetPosition, camera.targetVelocity, dt);
                    }
                    else
                    {
                        camera.lastTarget = camera.target;
                        camera.targetPosition = targetPosition;
                    }
                }
                else
                {
                    camera.lastTarget = camera.target;
                }
            }
            else
            {
                camera.targetPosition = camera.target.getComponent<xy::Transform>().getWorldPosition();
            }

            sf::Vector2f targetPos(smoothMotion(tx.getPosition(), camera.targetPosition, camera.velocity, dt));

            //clamp to bounds
            auto offset = entity.getComponent<xy::Camera>().getView() / 2.f;
            targetPos.x = xy::Util::Math::clamp(targetPos.x, m_bounds.left + offset.x, (m_bounds.left + m_bounds.width) - offset.x);
            targetPos.y = xy::Util::Math::clamp(targetPos.y, m_bounds.top + offset.y, (m_bounds.top + m_bounds.height) - offset.y);
            
            //add any shake
            camera.shakeAmount = std::max(camera.shakeAmount - dt, 0.f);
            targetPos.x += camera.shakeAmount * m_shakeTable[m_shakeIndex];

            targetPos.x = xy::Util::Math::round(targetPos.x);
            targetPos.y = xy::Util::Math::round(targetPos.y);

            tx.setPosition(targetPos);
        }
    }

    //update wavetable
    m_shakeIndex = (m_shakeIndex + 1) % m_shakeTable.size();
}