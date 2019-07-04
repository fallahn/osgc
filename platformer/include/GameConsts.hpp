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

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstdint>
#include <array>

namespace GameConst
{
    static const std::int32_t BackgroundDepth = -50;
    static const std::int32_t TextDepth = 10;
    static const float PixelsPerTile = 64.f; //world pixels per game tile - eg tiles are scaled up to this

    static const float VerticalTileCount = 17.f; //only true for 16px tiles :/

    namespace Gearboy
    {
        static const sf::FloatRect PlayerBounds(-6.f, -10.f, 12.f, 10.f); //relative to player origin
        static const sf::FloatRect PlayerFoot(-5.5f, 0.f, 11.f, 4.f);
        static const sf::FloatRect PlayerLeftHand(-7.f, -10.f, 1.f, 12.f);
        static const sf::FloatRect PlayerRightHand(6.f, -10.f, 1.f, 12.f);

        static const sf::Vector2f StarOffset(0.f, -20.f);
        static const float StarSpeed = 400.f;

        static const std::array<sf::Color, 4u> colours =
        {
            sf::Color(215,232,148),
            sf::Color(174,196,64),
            sf::Color(82,127,57),
            sf::Color(32,70,49)
        };
    }
}