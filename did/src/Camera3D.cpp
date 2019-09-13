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

#include "Camera3D.hpp"
#include "MessageIDs.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Wavetable.hpp>

namespace
{
    const float CameraSpeed = 2.5f;
    const float SpeedUpDistance = 250.f; //when the camera is within this distance of the target it gets sped up slightly

    const float SlackRadius = 8.f * 8.f;
}

Camera3DSystem::Camera3DSystem(xy::MessageBus& mb)
    : xy::System    (mb, typeid(Camera3DSystem)),
    m_chaseEnabled  (false),
    m_lockedOn      (false),
    m_active        (true),
    m_shakeIndex    (0)
{
    requireComponent<Camera3D>();

    m_shakeTable = xy::Util::Wavetable::sine(12.f, 2.f);
}

//public
void Camera3DSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto& camera = entity.getComponent<Camera3D>();
        auto txPosition = tx.getPosition();

        if (m_active)
        {
            if (camera.target.isValid() && m_chaseEnabled)
            {
                auto destPosition = camera.target.getComponent<xy::Transform>().getPosition();
                destPosition.y += Global::PlayerCameraOffset;

                auto movement = destPosition - txPosition;
                auto len2 = xy::Util::Vector::lengthSquared(movement);

                if (!m_lockedOn)
                {
                    //move a bit faster when we get nearer
                    float multiplier = 1.f - (std::min(SpeedUpDistance, len2) / SpeedUpDistance);
                    multiplier *= 2.f;

                    tx.move(movement * (CameraSpeed * (1.f + multiplier)) * dt);

                    if (len2 < 1)
                    {
                        m_lockedOn = true;

                        auto* msg = postMessage<SceneEvent>(MessageID::SceneMessage);
                        msg->type = SceneEvent::CameraLocked;
                    }
                }
                else
                {
                    auto pos = tx.getPosition();
                    pos.x = destPosition.x;

                    if (movement.y < 0)
                    {
                        if (len2 > SlackRadius)
                        {
                            //move a bit faster when we get nearer
                            float multiplier = 1.f - (std::min(SpeedUpDistance, len2) / SpeedUpDistance);
                            multiplier *= 2.f;

                            pos.y += (movement.y * (CameraSpeed * (1.f + multiplier)) * dt);
                        }
                    }
                    else
                    {
                        pos.y = destPosition.y;
                    }

                    tx.setPosition(pos);
                }

            }

            txPosition = tx.getPosition();
        }

        //shake it like a polaroid picture
        txPosition.x += m_shakeTable[m_shakeIndex] * camera.shakeAmount;
        camera.shakeAmount *= 0.92f;
        if (camera.shakeAmount < 0.1f)
        {
            camera.shakeAmount = 0.f;
        }
        tx.setPosition(txPosition);

        camera.worldPosition = glm::vec3(txPosition.x, camera.height, txPosition.y);
        camera.viewMatrix = glm::translate(glm::mat4(1.f), camera.worldPosition);
        camera.viewMatrix = glm::rotate(camera.viewMatrix, camera.pitch, glm::vec3(1.f, 0.f, 0.f));
        camera.viewMatrix = glm::inverse(camera.viewMatrix);

        camera.viewProjectionMatrix = camera.projectionMatrix * camera.viewMatrix;
    }

    m_shakeIndex = (m_shakeIndex + 1) % m_shakeTable.size();
}

void Camera3DSystem::enableChase(bool enable)
{
    m_chaseEnabled = enable;
    m_lockedOn = false;

    auto* msg = postMessage<SceneEvent>(MessageID::SceneMessage);
    msg->type = SceneEvent::CameraUnlocked;
}