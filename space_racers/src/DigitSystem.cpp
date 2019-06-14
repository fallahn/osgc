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

#include "DigitSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>

namespace
{
    //TODO ideally we want to get this from the texture
    const sf::FloatRect frame(0.f, 0.f, 25.f, 40.f);
    const float ScrollSpeed = 6.f;
}

DigitSystem::DigitSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(DigitSystem))
{
    requireComponent<Digit>();
    requireComponent<xy::Drawable>();
}

//public
void DigitSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& digit = entity.getComponent<Digit>();
        if (digit.lastValue != digit.value)
        {
            //update new target
            std::int32_t diff = digit.value - digit.lastValue;
            float tens = static_cast<float>(std::min(9, digit.value / 10));
            digit.targetTenPosition = frame.height * tens; //this column never rolls over, so woo

            float units = static_cast<float>(digit.value % 10);
            float lastUnits = static_cast<float>(digit.lastValue % 10);

            digit.targetUnitPosition = (frame.height * units) + ((frame.height * 10.f) * tens);
        }
        digit.lastValue = digit.value;

        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();

        //move towards target
        float amount = digit.targetTenPosition - verts[0].texCoords.y;
        amount *= dt * ScrollSpeed;

        for (auto i = 0; i < 4; ++i)
        {
            verts[i].texCoords.y += amount;
        }

        amount = digit.targetUnitPosition - verts[4].texCoords.y;
        amount *= dt * ScrollSpeed;

        for (auto i = 4; i < 8; ++i)
        {
            verts[i].texCoords.y += amount;
        }

        //update vertex array
        drawable.updateLocalBounds();
    }
}

//private
void DigitSystem::onEntityAdded(xy::Entity entity)
{
    auto& drawable = entity.getComponent<xy::Drawable>();
    auto& verts = drawable.getVertices();

    //tens
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(frame.width, 0.f), sf::Vector2f(frame.width, 0.f));
    verts.emplace_back(sf::Vector2f(frame.width, frame.height), sf::Vector2f(frame.width, frame.height));
    verts.emplace_back(sf::Vector2f(0.f, frame.height), sf::Vector2f(0.f, frame.height));

    //units
    verts.emplace_back(sf::Vector2f(frame.width, 0.f), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(frame.width * 2.f, 0.f), sf::Vector2f(frame.width, 0.f));
    verts.emplace_back(sf::Vector2f(frame.width * 2.f, frame.height), sf::Vector2f(frame.width, frame.height));
    verts.emplace_back(sf::Vector2f(frame.width, frame.height), sf::Vector2f(0.f, frame.height));

    //frames
    verts.emplace_back(sf::Vector2f(), sf::Vector2f(frame.width, 0.f));
    verts.emplace_back(sf::Vector2f(frame.width, 0.f), sf::Vector2f(frame.width * 2.f, 0.f));
    verts.emplace_back(sf::Vector2f(frame.width, frame.height), sf::Vector2f(frame.width * 2.f, frame.height));
    verts.emplace_back(sf::Vector2f(0.f, frame.height), sf::Vector2f(frame.width, frame.height));

    verts.emplace_back(sf::Vector2f(frame.width, 0.f), sf::Vector2f(frame.width, 0.f));
    verts.emplace_back(sf::Vector2f(frame.width * 2.f, 0.f), sf::Vector2f(frame.width * 2.f, 0.f));
    verts.emplace_back(sf::Vector2f(frame.width * 2.f, frame.height), sf::Vector2f(frame.width * 2.f, frame.height));
    verts.emplace_back(sf::Vector2f(frame.width, frame.height), sf::Vector2f(frame.width, frame.height));

    drawable.updateLocalBounds();
}