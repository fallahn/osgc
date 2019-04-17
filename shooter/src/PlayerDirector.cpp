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
#include "CommandIDs.hpp"
#include "Drone.hpp"

#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/util/Vector.hpp>

#include <SFML/Window/Event.hpp>

namespace
{
    const float Acceleration = 60.f;
}

PlayerDirector::PlayerDirector()
    : m_inputFlags(0)
{

}

//public
void PlayerDirector::handleEvent(const sf::Event& evt)
{
    //TODO input mappings
    if (evt.type == sf::Event::KeyPressed)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::Up:
        case sf::Keyboard::W:
            m_inputFlags |= Flags::Up;
            break;
        case sf::Keyboard::Down:
        case sf::Keyboard::S:
            m_inputFlags |= Flags::Down;
            break;
        case sf::Keyboard::Left:
        case sf::Keyboard::A:
            m_inputFlags |= Flags::Left;
            break;
        case sf::Keyboard::Right:
        case sf::Keyboard::D:
            m_inputFlags |= Flags::Right;
            break;
        }
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::Up:
        case sf::Keyboard::W:
            m_inputFlags &= ~Flags::Up;
            break;
        case sf::Keyboard::Down:
        case sf::Keyboard::S:
            m_inputFlags &= ~Flags::Down;
            break;
        case sf::Keyboard::Left:
        case sf::Keyboard::A:
            m_inputFlags &= ~Flags::Left;
            break;
        case sf::Keyboard::Right:
        case sf::Keyboard::D:
            m_inputFlags &= ~Flags::Right;
            break;
        }
    }
}

void PlayerDirector::handleMessage(const xy::Message&)
{

}

void PlayerDirector::process(float)
{
    sf::Vector2f velocity;
    if (m_inputFlags & Flags::Up)
    {
        velocity.y -= 1.f;
    }
    if (m_inputFlags & Flags::Down)
    {
        velocity.y += 1.f;
    }
    if (m_inputFlags & Flags::Left)
    {
        velocity.x -= 1.f;
    }
    if (m_inputFlags & Flags::Right)
    {
        velocity.x += 1.f;
    }

    if (float len2 = xy::Util::Vector::lengthSquared(velocity); len2 > 1)
    {
        float len = std::sqrt(len2);
        velocity /= len2;
    }

    xy::Command cmd;
    cmd.targetFlags = CommandID::PlayerTop;
    cmd.action = [velocity](xy::Entity entity, float)
    {
        entity.getComponent<Drone>().velocity += velocity * Acceleration;
    };
    sendCommand(cmd);
}