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
#include "StateIDs.hpp"

#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/util/Vector.hpp>

#include <SFML/Window/Event.hpp>

namespace
{

}

PlayerDirector::PlayerDirector(const KeyMap& keymap)
    : m_keymap  (keymap),
    m_inputFlags(0)
{

}

//public
void PlayerDirector::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyPressed)
    {
        if (evt.key.code == m_keymap.up)
        {
            m_inputFlags |= Drone::Up;
        }
        else if (evt.key.code == m_keymap.down)
        {
            m_inputFlags |= Drone::Down;
        }
        else if (evt.key.code == m_keymap.left)
        {
            m_inputFlags |= Drone::Left;
        }
        else if (evt.key.code == m_keymap.right)
        {
            m_inputFlags |= Drone::Right;
        }
        else if (evt.key.code == m_keymap.pickup)
        {
            m_inputFlags |= Drone::Pickup;
        }
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == m_keymap.up)
        {
            m_inputFlags &= ~Drone::Up;
        }
        else if (evt.key.code == m_keymap.down)
        {
            m_inputFlags &= ~Drone::Down;
        }
        else if (evt.key.code == m_keymap.left)
        {
            m_inputFlags &= ~Drone::Left;
        }
        else if (evt.key.code == m_keymap.right)
        {
            m_inputFlags &= ~Drone::Right;
        }
        else if (evt.key.code == m_keymap.pickup)
        {
            m_inputFlags &= ~Drone::Pickup;
        }
        else if (evt.key.code == m_keymap.fire)
        {
            m_inputFlags |= Drone::Fire;
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed
        && evt.joystickButton.joystickId == 0)
    {
        if (evt.joystickButton.button == m_keymap.joyFire)
        {
            m_inputFlags |= Drone::Fire;
        }
        else if (evt.joystickButton.button == m_keymap.joyPickup)
        {
            m_inputFlags |= Drone::Pickup;
        }
    }
    else if (evt.type == sf::Event::JoystickMoved
        && evt.joystickMove.joystickId == 0)
    {
        if (evt.joystickMove.axis == sf::Joystick::PovX
            || evt.joystickMove.axis == sf::Joystick::X)
        {
            if (evt.joystickMove.position > 40.f)
            {
                m_inputFlags |= Drone::Right;
                m_inputFlags &= ~Drone::Left;
            }
            else if (evt.joystickMove.position < -40.f)
            {
                m_inputFlags |= Drone::Left;
                m_inputFlags &= ~Drone::Right;
            }
            else
            {
                m_inputFlags &= ~(Drone::Left | Drone::Right);
            }
        }
        else if (evt.joystickMove.axis == sf::Joystick::PovY)
        {
#ifdef WIN32 //this axis is inverted on win32
            if (evt.joystickMove.position > 40.f)
            {
                m_inputFlags |= Drone::Up;
                m_inputFlags &= ~Drone::Down;
            }
            else if (evt.joystickMove.position < -40.f)
            {
                m_inputFlags |= Drone::Down;
                m_inputFlags &= ~Drone::Up;
            }
            else
            {
                m_inputFlags &= ~(Drone::Down | Drone::Up);
            }
#else
            if (evt.joystickMove.position > 40.f)
            {
                m_inputFlags |= Drone::Down;
                m_inputFlags &= ~Drone::Up;
            }
            else if (evt.joystickMove.position < -40.f)
            {
                m_inputFlags |= Drone::Up;
                m_inputFlags &= ~Drone::Down;
            }
            else
            {
                m_inputFlags &= ~(Drone::Down | Drone::Up);
            }
#endif
        }
        else if (evt.joystickMove.axis == sf::Joystick::Y)
        {
            if (evt.joystickMove.position > 40.f)
            {
                m_inputFlags |= Drone::Down;
                m_inputFlags &= ~Drone::Up;
            }
            else if (evt.joystickMove.position < -40.f)
            {
                m_inputFlags |= Drone::Up;
                m_inputFlags &= ~Drone::Down;
            }
            else
            {
                m_inputFlags &= ~(Drone::Down | Drone::Up);
            }
        }
    }
}

void PlayerDirector::handleMessage(const xy::Message&)
{

}

void PlayerDirector::process(float)
{
    auto flags = m_inputFlags;

    xy::Command cmd;
    cmd.targetFlags = CommandID::PlayerTop;
    cmd.action = [flags](xy::Entity entity, float)
    {
        entity.getComponent<Drone>().inputFlags = flags;
    };
    sendCommand(cmd);

    //make sure to reset fire/pickup button
    m_inputFlags &= ~(Drone::Fire | Drone::Pickup);
}