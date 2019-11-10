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

#include "PlayerDirector.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace
{
    //TODO bindable keymap
    const sf::Keyboard::Key left = sf::Keyboard::A;
    const sf::Keyboard::Key right = sf::Keyboard::D;
    const sf::Keyboard::Key up = sf::Keyboard::W; //pressing up inspects something behind the player

    enum InputFlags
    {
        Up = 0x1,
        Left = 0x2,
        Right = 0x4
    };

    const float PlayerSpeed = 180.f;
}

PlayerDirector::PlayerDirector()
    : m_cameraLocked    (false),
    m_cameraDirection   (0),
    m_inputFlags        (0)
{

}

//public
void PlayerDirector::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyPressed)
    {
        switch (evt.key.code)
        {
        default: break;
        case left:
            m_inputFlags |= Left;
            break;
        case right:
            m_inputFlags |= Right;
            break;
        case up:
            m_inputFlags |= Up;
            break;
        }
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case left:
            m_inputFlags &= ~Left;
            break;
        case right:
            m_inputFlags &= ~Right;
            break;
        case up:
            m_inputFlags &= ~Up;
            break;
        }
    }
}

void PlayerDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::CameraMessage)
    {
        const auto& data = msg.getData<CameraEvent>();
        if (data.type == CameraEvent::Locked)
        {
            m_cameraLocked = true;
            m_cameraDirection = data.direction;
        }
        else if (data.type == CameraEvent::Unlocked)
        {
            m_cameraLocked = false;
        }
    }
}

void PlayerDirector::process(float dt)
{
    if (m_cameraLocked && m_playerEntity.isValid())
    {
        if (m_inputFlags != 0)
        {
            sf::Vector2f velocity;
            if (m_inputFlags & InputFlags::Left)
            {
                switch (m_cameraDirection)
                {
                default: break;
                case CameraEvent::N:
                    velocity.x = -1.f;
                    break;
                case CameraEvent::E:
                    velocity.y = -1.f;
                    break;
                case CameraEvent::S:
                    velocity.x = 1.f;
                    break;
                case CameraEvent::W:
                    velocity.y = 1.f;
                    break;
                }
            }

            if (m_inputFlags & InputFlags::Right)
            {
                switch (m_cameraDirection)
                {
                default: break;
                case CameraEvent::N:
                    velocity.x = 1.f;
                    break;
                case CameraEvent::E:
                    velocity.y = 1.f;
                    break;
                case CameraEvent::S:
                    velocity.x = -1.f;
                    break;
                case CameraEvent::W:
                    velocity.y = -1.f;
                    break;
                }
            }

            m_playerEntity.getComponent<xy::Transform>().move(velocity * PlayerSpeed * dt);

            //TODO inspect stuff if UP is released
        }
    }
}