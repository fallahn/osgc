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

#include <cstdint>

namespace GameConst
{
    static const std::int32_t BackgroundDepth = -50;
    static const float PixelsPerTile = 64.f; //world pixels per game tile - eg tiles are scaled up to this

    namespace Gearboy
    {
        static const sf::FloatRect PlayerBounds(-6.f, -10.f, 12.f, 10.f); //relative to player origin
        static const sf::FloatRect PlayerFoot(-6.f, 0.f, 12.f, 4.f);
    }
}