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
#include "Packet.hpp"
#include "PlayerSystem.hpp"
#include "BotSystem.hpp"

#include <xyginext/ecs/Entity.hpp>
#include <xyginext/core/App.hpp>
#include <xyginext/network/NetClient.hpp>

#include <SFML/Window/Event.hpp>

#include <cmath>

namespace
{
    const float deadZone = 20.f;
}

InputParser::InputParser(const InputBinding& ib, xy::NetClient& connection)
    : m_currentInput    (0),
    m_prevPad           (0),
    m_prevStick         (0),
    m_analogueAmount    (1.f),
    m_timeAccumulator   (0),
    m_enabled           (false),
    m_playerEntity      (0,0),
    m_playerNumber      (255),
    m_inputBinding      (ib),
    m_connection        (connection)
{

}

//public
void InputParser::handleEvent(const sf::Event& evt)
{   
    //apply to input mask
    if (evt.type == sf::Event::KeyPressed)
    {
        if(evt.key.code == m_inputBinding.keys[InputBinding::Up])
        {
            m_currentInput |= InputFlag::Up;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Left])
        {
            m_currentInput |= InputFlag::Left;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Right])
        {
            m_currentInput |= InputFlag::Right;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Down])
        {
            m_currentInput |= InputFlag::Down;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::CarryDrop])
        {
            m_currentInput |= InputFlag::CarryDrop;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Action])
        {
            m_currentInput |= InputFlag::Action;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::WeaponPrev])
        {
            m_currentInput |= InputFlag::WeaponPrev;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::WeaponNext])
        {
            m_currentInput |= InputFlag::WeaponNext;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Strafe])
        {
            m_currentInput |= InputFlag::Strafe;
        }
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == m_inputBinding.keys[InputBinding::Up])
        {
            m_currentInput &= ~InputFlag::Up;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Left])
        {
            m_currentInput &= ~InputFlag::Left;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Right])
        {
            m_currentInput &= ~InputFlag::Right;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Down])
        {
            m_currentInput &= ~InputFlag::Down;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::CarryDrop])
        {
            m_currentInput &= ~InputFlag::CarryDrop;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Action])
        {
            m_currentInput &= ~InputFlag::Action;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::WeaponNext])
        {
            m_currentInput &= ~InputFlag::WeaponNext;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::WeaponPrev])
        {
            m_currentInput &= ~InputFlag::WeaponPrev;
        }
        else if (evt.key.code == m_inputBinding.keys[InputBinding::Strafe])
        {
            m_currentInput &= ~InputFlag::Strafe;
        }

    }
    else if (evt.type == sf::Event::JoystickButtonPressed)
    {
        if (evt.joystickButton.joystickId == m_inputBinding.controllerID)
        {
            if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Action])
            {
                m_currentInput |= InputFlag::Action;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::CarryDrop])
            {
                m_currentInput |= InputFlag::CarryDrop;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::WeaponNext])
            {
                m_currentInput |= InputFlag::WeaponNext;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::WeaponPrev])
            {
                m_currentInput |= InputFlag::WeaponPrev;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Strafe])
            {
                m_currentInput |= InputFlag::Strafe;
            }
        }
    }
    else if (evt.type == sf::Event::JoystickButtonReleased)
    {
        if (evt.joystickButton.joystickId == m_inputBinding.controllerID)
        {
            if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Action])
            {
                m_currentInput &= ~InputFlag::Action;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::CarryDrop])
            {
                m_currentInput &= ~InputFlag::CarryDrop;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::WeaponNext])
            {
                m_currentInput &= ~InputFlag::WeaponNext;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::WeaponPrev])
            {
                m_currentInput &= ~InputFlag::WeaponPrev;
            }
            else if (evt.joystickButton.button == m_inputBinding.buttons[InputBinding::Strafe])
            {
                m_currentInput &= ~InputFlag::Strafe;
            }
        }
    }
}

void InputParser::update(float dt)
{
    m_timeAccumulator += static_cast<std::int32_t>(dt * 1000000.f);

    if (m_playerEntity.getIndex() == 0) return;

    //check state of controller axis
    checkControllerInput();

    //check to use bot data
    const auto& bot = m_playerEntity.getComponent<Bot>();
    if (bot.enabled)
    {
        m_currentInput = bot.inputMask;
    }

    Input input;
    input.mask = m_enabled ? m_currentInput : 0;
    //input.timestamp = m_clientTimer.getElapsedTime().asMicroseconds();
    input.timestamp = m_timeAccumulator;
    input.acceleration = m_analogueAmount;

    //update player input history
    if (!bot.enabled)
    {
        auto& inputComponent = m_playerEntity.getComponent<InputComponent>();
        inputComponent.history[inputComponent.currentInput].input = input;
        inputComponent.currentInput = (inputComponent.currentInput + 1) % inputComponent.history.size();
    }

    //send input to server
    InputUpdate iu;
    iu.clientTime = input.timestamp;
    iu.input = input.mask;
    iu.playerNumber = m_playerNumber;
    iu.acceleration = input.acceleration;

    m_connection.sendPacket(PacketID::ClientInput, iu, xy::NetFlag::Unreliable);
}

void InputParser::setPlayerEntity(xy::Entity entity, std::uint8_t playerNumber)
{
    m_playerEntity = entity;
    m_playerNumber = playerNumber;
    m_clientTimer.restart();
}

xy::Entity InputParser::getPlayerEntity() const
{
    return m_playerEntity;
}

void InputParser::setEnabled(bool enabled)
{
    m_enabled = enabled;
    m_currentInput = 0;
}

//private
void InputParser::checkControllerInput()
{
    m_analogueAmount = 1.f;

    if (m_inputBinding.controllerID > sf::Joystick::Count) return;
    if (!sf::Joystick::isConnected(m_inputBinding.controllerID)) return;

    auto startInput = m_currentInput;

    //DPad
    if (sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::PovX) < -deadZone)
    {
        m_currentInput |= InputFlag::Left;
    }
    else if (m_prevPad & InputFlag::Left)
    {
        m_currentInput &= ~InputFlag::Left;
    }

    if (sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::PovX) > deadZone)
    {
        m_currentInput |= InputFlag::Right;
    }
    else if (m_prevPad & InputFlag::Right)
    {
        m_currentInput &= ~InputFlag::Right;
    }

    //dpad axis is reversed on windows, hum
#ifdef _WIN32
    if (sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::PovY) < -deadZone)
    {
        m_currentInput |= InputFlag::Down;
        m_currentInput &= ~InputFlag::Up;
    }
#else
    if (sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::PovY) > deadZone)
    {
        m_currentInput |= InputFlag::Down;
        m_currentInput &= ~InputFlag::Up;
    }
#endif //_WIN32
    else if (m_prevPad & InputFlag::Down)
    {
        m_currentInput &= ~InputFlag::Down;
    }


#ifdef _WIN32
    if (sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::PovY) > deadZone)
    {
        m_currentInput |= InputFlag::Up;
        m_currentInput &= ~InputFlag::Down;
    }
#else
    if (sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::PovY) < -deadZone)
    {
        m_currentInput |= InputFlag::Up;
        m_currentInput &= ~InputFlag::Down;
    }
#endif //_WIN32
    else if (m_prevPad & InputFlag::Up)
    {
        m_currentInput &= ~InputFlag::Up;
    }

    if (startInput ^ m_currentInput)
    {
        m_prevPad = m_currentInput;
        return; //prevent analogue stick overwriting state
    }

    
    //left stick (xbox controller)
    float xPos = sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::X);
    if (xPos < -deadZone)
    {
        m_currentInput |= InputFlag::Left;
    }
    else if (m_prevStick & InputFlag::Left)
    {
        m_currentInput &= ~InputFlag::Left;
    }

    if (xPos > deadZone)
    {
        m_currentInput |= InputFlag::Right;
    }
    else if (m_prevStick & InputFlag::Right)
    {
        m_currentInput &= ~InputFlag::Right;
    }

    float yPos = sf::Joystick::getAxisPosition(m_inputBinding.controllerID, sf::Joystick::Y);
    if (yPos > (deadZone /** 3.5f*/))
    {
        m_currentInput |= InputFlag::Down;
        m_currentInput &= ~InputFlag::Up;
    }
    else if (m_prevStick & InputFlag::Down)
    {
        m_currentInput &= ~InputFlag::Down;
    }

    if (yPos < (-deadZone /** 3.5f*/))
    {
        m_currentInput |= InputFlag::Up;
        m_currentInput &= ~InputFlag::Down;
    }
    else if (m_prevStick & InputFlag::Up)
    {
        m_currentInput &= ~InputFlag::Up;
    }

    float len2 = (xPos * xPos) + (yPos * yPos);
    static const float MinLen2 = (deadZone * deadZone);
    if (len2 > MinLen2)
    {
        m_analogueAmount = std::sqrt(len2) / 100.f;
    }


    if (startInput ^ m_currentInput)
    {
        m_prevStick = m_currentInput;
    }
}