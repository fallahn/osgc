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

#pragma once

#include <xyginext/ecs/System.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

#include <array>

struct Trail final
{
    struct Point final
    {
        sf::Vector2f position;
        float lifetime = 0.f;
        bool sleeping = false;
        static constexpr float MaxLifetime = 0.4f;
    };

    std::array<Point, 6u> points;

    //make sure to track the index of the
    //oldest point as the vert array will start here
    std::size_t oldestPoint = 0;

    //when the sleep count reaches array size destoy this trail
    std::size_t sleepCount = 0;

    //entity we're following. If this becomes invalid
    //we stop resetting points, and start counting sleeping
    //points.
    xy::Entity parent;
    sf::Vector2f parentLastPosition;

    sf::Color colour = sf::Color::White;
};

class TrailSystem final : public xy::System 
{
public:
    explicit TrailSystem(xy::MessageBus&);

    void process(float) override;

private:
    void onEntityAdded(xy::Entity) override;
};
