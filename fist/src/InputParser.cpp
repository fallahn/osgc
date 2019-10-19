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

#include <xyginext/util/Vector.hpp>

#include <SFML/Window/Keyboard.hpp>

namespace
{
    const float PlayerStrength = 320.f;
    const float MaxVelocity = 1800.f;
    const float MaxVelSqr = MaxVelocity * MaxVelocity;
}

InputParser::InputParser()
{

}

//public
void InputParser::update()
{
    if (m_playerEntity.isValid())
    {
        sf::Vector2f movement;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {
            movement.x -= 1.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            movement.x += 1.f;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        {
            movement.y -= 1.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        {
            movement.y += 1.f;
        }
        if (xy::Util::Vector::lengthSquared(movement) > 1)
        {
            movement = xy::Util::Vector::normalise(movement);
        }

        movement *= PlayerStrength;

        auto& vel = m_playerEntity.getComponent<sf::Vector2f>();
        if (xy::Util::Vector::lengthSquared(vel) < MaxVelSqr)
        {
            vel += movement;
        }
    }
}