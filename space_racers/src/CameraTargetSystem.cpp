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
    const float CameraSpeed = 5.5f;
    const float SpeedUpDistance = 350.f; //when the camera is within this distance of the target it gets sped up slightly

    const float SlackRadius = 100.f * 100.f;
    const float AspectMultiplier = 9.f / 16.f;
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
        auto destPosition = camera.target.getComponent<xy::Transform>().getPosition();
        
        if (camera.target.isValid() && camera.active)
        {
            auto txPosition = tx.getPosition();
            auto movement = destPosition - txPosition;
            auto len2 = xy::Util::Vector::lengthSquared(movement);

            if (!camera.lockedOn)
            {
                //move a bit faster when we get nearer
                float multiplier = 1.f - (std::min(SpeedUpDistance, len2) / SpeedUpDistance);
                multiplier *= 2.f;

                tx.move(movement * (CameraSpeed * (1.f + multiplier)) * dt);

                if (len2 < 2.f)
                {
                    camera.lockedOn = true;

                    /*auto* msg = postMessage<SceneEvent>(MessageID::SceneMessage);
                    msg->type = SceneEvent::CameraLocked;*/
                }
            }
            else
            {
                //TODO we could add some slight slack here, but it might
                //also cause some motion sickness
                tx.setPosition(destPosition);
            }
        }
    }
}