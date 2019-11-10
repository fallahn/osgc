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
#include "MessageIDs.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/util/Math.hpp>
#include <xyginext/util/Vector.hpp>

#include <array>

namespace
{
    const float TranslateSpeed = 10.f;
    const float RotateSpeed = 20.f;

    const std::array<float, 4u> rotationTargets =
    {
        0.f, -90.f, 180.f, 90.f
    };
}

CameraTransport::CameraTransport(std::int32_t startingRoom)
{
    auto x = startingRoom % GameConst::RoomsPerRow;
    auto y = startingRoom / GameConst::RoomsPerRow;

    targetPosition = { x * GameConst::RoomWidth, y * -GameConst::RoomWidth };
    currentPosition = targetPosition - sf::Vector2f(2.f, 2.f);
    active = true; //this just makes the camera snap to initial position
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

        auto* msg = xy::App::getActiveInstance()->getMessageBus().post<CameraEvent>(MessageID::CameraMessage);
        msg->type = CameraEvent::Unlocked;
    }
}

void CameraTransport::rotate(bool left)
{
    if (!active)
    {
        if (left)
        {
            m_currentRotationTarget = (m_currentRotationTarget + 3) % rotationTargets.size();

            float temp = currentDirection.x;
            currentDirection.x = -currentDirection.y;
            currentDirection.y = temp;
        }
        else
        {
            m_currentRotationTarget = (m_currentRotationTarget + 1) % rotationTargets.size();

            float temp = currentDirection.y;
            currentDirection.y = -currentDirection.x;
            currentDirection.x = temp;
        }

        targetRotation = rotationTargets[m_currentRotationTarget];
        active = true;

        auto* msg = xy::App::getActiveInstance()->getMessageBus().post<CameraEvent>(MessageID::CameraMessage);
        msg->type = CameraEvent::Unlocked;
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

                    //raise a message
                    auto* msg = postMessage<CameraEvent>(MessageID::CameraMessage);
                    msg->type = CameraEvent::Locked;
                    msg->direction = static_cast<CameraEvent::Direction>(transport.m_currentRotationTarget);
                }
                entity.getComponent<Camera3D>().postTranslationMatrix = glm::translate(glm::mat4(1.f), glm::vec3(-transport.currentPosition.x, transport.currentPosition.y, 0.f));
            }


            //rotation
            float rotation = xy::Util::Math::shortestRotation(transport.currentRotation, transport.targetRotation);
            if (rotation != 0)
            {
                if (std::abs(rotation) > 1)
                {
                    float amount = rotation * RotateSpeed * dt;
                    auto& cam = entity.getComponent<Camera3D>();
                    cam.postRotationMatrix = glm::rotate(cam.postRotationMatrix, amount * xy::Util::Const::degToRad, glm::vec3(0.f, 0.f, 1.f));

                    transport.currentRotation += amount;
                }
                else
                {
                    auto& cam = entity.getComponent<Camera3D>();
                    cam.postRotationMatrix = glm::rotate(glm::mat4(1.f), transport.targetRotation * xy::Util::Const::degToRad, glm::vec3(0.f, 0.f, 1.f));
                    transport.currentRotation = transport.targetRotation;
                    transport.active = false;

                    //raise message
                    auto* msg = postMessage<CameraEvent>(MessageID::CameraMessage);
                    msg->type = CameraEvent::Locked;
                    msg->direction = static_cast<CameraEvent::Direction>(transport.m_currentRotationTarget);
                }
            }
        }
    }
}