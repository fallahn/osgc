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

#include "InputParser.hpp"
#include "InputBinding.hpp"
#include "Player.hpp"

#include <SFML/Window/Event.hpp>

namespace
{
    const float Deadzone = 30.f;
}

InputParser::InputParser(const InputBinding& binding)
    : m_inputBinding        (binding),
    m_runningMultiplier     (1.f),
    m_currentInput          (0)
{

}

//public
void InputParser::handleEvent(const sf::Event& evt)
{
    //apply to input mask
    if (evt.type == sf::Event::KeyPressed)
    {
        if (evt.key.code == m_inputBinding.keys[InputBinding::Jump])
        {
            m_currentInput |= InputFlag::Jump;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Left])
        {
            m_currentInput |= InputFlag::Left;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Right])
        {
            m_currentInput |= InputFlag::Right;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Shoot])
        {
            m_currentInput |= InputFlag::Shoot;
        }
        /*else if (evt.key.code == m_inputBinding.keys[InputBinding::Reverse])
        {
            m_currentInput |= InputFlag::Reverse;
        }*/
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == m_inputBinding.keys[InputBinding::Jump])
        {
            m_currentInput &= ~InputFlag::Jump;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Left])
        {
            m_currentInput &= ~InputFlag::Left;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Right])
        {
            m_currentInput &= ~InputFlag::Right;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Shoot])
        {
            m_currentInput &= ~InputFlag::Shoot;
        }
        /*else if (evt.key.code == m_inputBinding.keys[InputBinding::Reverse])
        {
            m_currentInput &= ~InputFlag::Reverse;
        }*/
    }
    else if (evt.type == sf::Event::JoystickButtonPressed)
    {
        if (evt.joystickButton.joystickId == m_inputBinding.controllerID)
        {
            if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Jump])
            {
                m_currentInput |= InputFlag::Jump;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Shoot])
            {
                m_currentInput |= InputFlag::Shoot;
            }
            /*else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Reverse])
            {
                m_currentInput |= InputFlag::Reverse;
            }*/
        }
    }
    else if (evt.type == sf::Event::JoystickButtonReleased)
    {
        if (evt.joystickButton.joystickId == m_inputBinding.controllerID)
        {
            if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Jump])
            {
                //prevent this overriding the z-axis state
                float z = sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::Z);
                if (z > /*-Deadzone && z < Deadzone*/-5.f)
                {
                    m_currentInput &= ~InputFlag::Jump;
                }
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Shoot])
            {
                m_currentInput &= ~InputFlag::Shoot;
            }
            /*else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Reverse])
            {
                m_currentInput &= ~InputFlag::Reverse;
            }*/
        }
    }
    else if (evt.type == sf::Event::JoystickMoved
        && evt.joystickMove.joystickId == m_inputBinding.controllerID)
    {
        if (evt.joystickMove.axis == sf::Joystick::PovX
            || evt.joystickMove.axis == sf::Joystick::X)
        {
            if (evt.joystickMove.position > Deadzone)
            {
                m_currentInput |= InputFlag::Right;
                m_currentInput &= ~InputFlag::Left;

                m_runningMultiplier = evt.joystickMove.position / 100.f;
            }
            else if (evt.joystickMove.position < -Deadzone)
            {
                m_currentInput |= InputFlag::Left;
                m_currentInput &= ~InputFlag::Right;

                m_runningMultiplier = evt.joystickMove.position / -100.f;
            }
            else
            {
                m_currentInput &= ~(InputFlag::Left | InputFlag::Right);
            }
        }
        
        //else if (evt.joystickMove.axis == sf::Joystick::Z)
        //{
        //    if (evt.joystickMove.position < /*-Deadzone*/-5.f)
        //    {
        //        m_currentInput |= InputFlag::Accelerate;

        //        m_accelerationMultiplier = evt.joystickMove.position / -100.f;
        //    }
        //    else
        //    {
        //        //prevent this overriding button down state
        //        if (!sf::Joystick::isButtonPressed(m_inputBinding.controllerID, m_inputBinding.buttons[InputBinding::Accelerate]))
        //        {
        //            m_currentInput &= ~InputFlag::Accelerate;
        //        }
        //    }
        //}
    }
}

void InputParser::update()
{
    if (m_playerEntity.isValid())
    {
        //set local player input
        auto& player = m_playerEntity.getComponent<Player>();

        //update player input history
        player.input = m_currentInput;
        player.accelerationMultiplier = m_runningMultiplier;

        //reset analogue multiplier
        //this has to be set at one for keyboard input
        if ((m_currentInput & (InputFlag::Left | InputFlag::Right)) == 0)
        {
            m_runningMultiplier = 1.f;
        }
    } 
}
