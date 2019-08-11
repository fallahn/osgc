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

#include "PlayerInput.hpp"
#include "MessageIDs.hpp"
#include "GameConsts.hpp"
#include "NodeSet.hpp"

#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/core/App.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

namespace
{
    const float MinAngle = 200.f;
    const float MaxAngle = 340.f;

    float mouseSpeed = 0.8f;
}

PlayerInput::PlayerInput(const NodeSet& ns)
    : m_nodeSet(ns),
    m_lastMousePos(sf::Mouse::getPosition().x)
{

}

//public
void PlayerInput::handleEvent(const sf::Event& evt)
{
    auto shoot = [&]()
    {
        //raise shoot message
        float rotation = m_playerEntity.getComponent<xy::Transform>().getRotation();

        auto* msg = xy::App::getActiveInstance()->getMessageBus().post<BubbleEvent>(MessageID::BubbleMessage);
        msg->type = BubbleEvent::Fired;
        msg->velocity = xy::Util::Vector::rotate({ 1.f, 0.f }, rotation);
    };

    if (evt.type == sf::Event::MouseButtonReleased
        && evt.mouseButton.button == sf::Mouse::Left)
    {
        shoot();
    }
    else if (evt.type == sf::Event::MouseMoved)
    {
        auto pos = evt.mouseMove.x;
        auto delta = pos - m_lastMousePos;
        m_lastMousePos = pos;
        rotatePlayer(float(delta) * mouseSpeed);
    }
    else if (evt.type == sf::Event::KeyPressed
        && evt.key.code == sf::Keyboard::Space)
    {
        shoot();
    }
}

void PlayerInput::update(float)
{
    const float rotationAmount = 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
    {
        rotatePlayer(-rotationAmount);
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
    {
        rotatePlayer(rotationAmount);
    }
}

//private
void PlayerInput::rotatePlayer(float amount)
{
    auto rotation = m_playerEntity.getComponent<xy::Transform>().getRotation();
    rotation = xy::Util::Math::clamp(rotation + amount, MinAngle, MaxAngle);
    m_playerEntity.getComponent<xy::Transform>().setRotation(rotation);
}