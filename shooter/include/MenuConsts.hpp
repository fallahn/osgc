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

namespace Menu
{
    static const sf::Vector2f TitlePosition(120.f, 80.f);
    static const std::uint32_t TitleCharSize = 120;

    static const sf::Vector2f ItemFirstPosition(120.f, 500.f);
    static const float ItemVerticalSpacing = 80.f;
    static const std::uint32_t ItemCharSize = 80;
}