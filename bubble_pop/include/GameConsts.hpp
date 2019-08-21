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

#include <SFML/System/Vector2.hpp>

#include <cstdint>
#include <cmath>

namespace Const
{
    static const sf::Vector2f BubbleSize(47.f, 47.f);
    static const float BubbleDistSqr = std::pow(BubbleSize.x * 0.84f, 2.f);

    static const sf::Vector2f GunPosition(328.f, 445.f);
    static const sf::Vector2f BarPosition(128.f, 8.f);
    static const sf::Vector2f BubbleQueuePosition(BubbleSize.x, 445.f);

    static const std::size_t BubblesPerRow = 8;
    static const std::size_t MaxRows = 8;
    static const std::size_t GridSize = BubblesPerRow * MaxRows;
    static const float RowHeight = BubbleSize.y * 0.8889f;

    static const float LeftBounds = 127.f + (BubbleSize.x / 2.f);
    static const float RightBounds = 534.f - (BubbleSize.x / 2.f);

    static const float MaxBubbleHeight = 370.f;

    static const std::streamsize MaxNameChar = 16;
    static const std::streamsize MinNameSpace = 4;
    static const std::streamsize MaxScoreSize = 6;
}

static inline sf::Vector2f tileToWorldCoord(std::size_t index)
{
    auto xPos = index % Const::BubblesPerRow;
    auto yPos = index / Const::BubblesPerRow;

    float x = xPos * Const::BubbleSize.x;
    float y = yPos * Const::RowHeight;

    if (yPos % 2 == 0)
    {
        x += Const::BubbleSize.x / 2.f;
    }

    return { x,y };
}