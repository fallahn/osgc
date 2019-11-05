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

#include "CameraTransportSystem.hpp"
#include "Camera3D.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/util/Math.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float TranslateSpeed = 10.f;
    const float RotateSpeed = 10.f;
}

void CameraTransport::move(bool left)
{
    if (!active)
    {
        sf::Vector2f dir;
        if (left)
        {
            dir.y = -currentDirection.x;
            dir.x = currentDirection.y;
        }
        else
        {
            dir.x = -currentDirection.y;
            dir.y = currentDirection.x;
        }

        targetPosition += dir * GameConst::RoomWidth;
        active = true;
    }
}

void CameraTransport::rotate(bool left)
{
    if (!active)
    {
        if (left)
        {
            targetRotation += 90.f;
            float temp = currentDirection.x;
            currentDirection.x = -currentDirection.y;
            currentDirection.y = temp;
        }
        else
        {
            targetRotation -= 90.f;
            float temp = currentDirection.y;
            currentDirection.y = -currentDirection.x;
            currentDirection.x = temp;
        }
        active = true;
    }
}

CameraTransportSystem::CameraTransportSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(CameraTransportSystem))
{
    requireComponent<xy::Transform>();
    requireComponent<CameraTransport>();
    requireComponent<Camera3D>();
}

//public
void CameraTransportSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& transport = entity.getComponent<CameraTransport>();
        if (transport.active)
        {
            //TODO edge cases might mean both translation and rotation happen
            //at the same time, but here we're assuming they don't
            auto direction = transport.targetPosition - transport.currentPosition;
            auto len2 = xy::Util::Vector::lengthSquared(direction);

            if (len2 > 0)
            {
                if (len2 > 25)
                {
                    transport.currentPosition += (direction * TranslateSpeed * dt);
                }
                else
                {
                    transport.currentPosition = transport.targetPosition;
                    transport.active = false;

                    //TODO raise a message
                }
                entity.getComponent<Camera3D>().postTranslationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(-transport.currentPosition.x, transport.currentPosition.y, 0.f));
            }


            //rotation
            float rotation = xy::Util::Math::shortestRotation(transport.currentRotation, transport.targetRotation);
            if (rotation != 0)
            {
                if (std::abs(rotation) > 2)
                {
                    float amount = rotation * RotateSpeed * dt;
                    auto& cam = entity.getComponent<Camera3D>();
                    cam.postRotationMatrix = glm::rotate(cam.postRotationMatrix, amount * xy::Util::Const::degToRad, glm::vec3(0.f, 0.f, 1.f));

                    transport.currentRotation += amount;
                }
                else
                {
                    auto& cam = entity.getComponent<Camera3D>();
                    cam.postRotationMatrix = glm::rotate(cam.postRotationMatrix, rotation * xy::Util::Const::degToRad, glm::vec3(0.f, 0.f, 1.f));
                    transport.currentRotation = transport.targetRotation;
                    transport.active = false;

                    //TODO raise message
                }
            }
        }
    }
}