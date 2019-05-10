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
#include "VehicleSystem.hpp"

#include <SFML/Window/Event.hpp>

InputParser::InputParser(xy::Entity entity, const InputBinding& binding)
    : m_playerEntity    (entity),
    m_inputBinding      (binding),
    m_currentInput      (0)
{

}

//public
void InputParser::handleEvent(const sf::Event& evt)
{
    //apply to input mask
    if (evt.type == sf::Event::KeyPressed)
    {
        if (evt.key.code == m_inputBinding.keys[InputBinding::Accelerate])
        {
            m_currentInput |= InputFlag::Accelerate;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Left])
        {
            m_currentInput |= InputFlag::Left;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Right])
        {
            m_currentInput |= InputFlag::Right;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Brake])
        {
            m_currentInput |= InputFlag::Brake;
        }
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == m_inputBinding.keys[InputBinding::Accelerate])
        {
            m_currentInput &= ~InputFlag::Accelerate;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Left])
        {
            m_currentInput &= ~InputFlag::Left;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Right])
        {
            m_currentInput &= ~InputFlag::Right;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Brake])
        {
            m_currentInput &= ~InputFlag::Brake;
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed)
    {
        if (evt.joystickButton.joystickId == m_inputBinding.controllerID)
        {
            if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Accelerate])
            {
                m_currentInput |= InputFlag::Accelerate;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Brake])
            {
                m_currentInput |= InputFlag::Brake;
            }
        }
    }
    else if (evt.type == sf::Event::JoystickButtonReleased)
    {
        if (evt.joystickButton.joystickId == m_inputBinding.controllerID)
        {
            if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Accelerate])
            {
                m_currentInput &= ~InputFlag::Accelerate;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Brake])
            {
                m_currentInput &= ~InputFlag::Brake;
            }
        }
    }

    //TODO joystick moved events (and analogue velocity)
}

void InputParser::update()
{
    if (m_playerEntity.isValid())
    {
        //set local player input
        auto& vehicle = m_playerEntity.getComponent<Vehicle>();

        Input input;
        input.flags = m_currentInput;
        input.timestamp = m_clientClock.getElapsedTime().asMicroseconds();

        //update player input history
        vehicle.history[vehicle.currentInput] = input;
        vehicle.currentInput = (vehicle.currentInput + 1) % vehicle.history.size();

        //TODO send input to server
        /*InputUpdate iu;
        iu.clientTime = input.timestamp;
        iu.input = input.mask;
        iu.playerNumber = player.playerNumber;

        m_netClient.sendPacket<InputUpdate>(PacketID::ClientInput, iu, xy::NetFlag::Reliable, 0);*/
    }
}