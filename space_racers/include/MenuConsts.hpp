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

#include <xyginext/Config.hpp>

#include <SFML/System/Vector2.hpp>

namespace MenuConst
{
    static const sf::Vector2f MainMenuPosition;
    static const sf::Vector2f NetworkMenuPosition(-xy::DefaultSceneSize.x, 0.f);
    static const sf::Vector2f ItemRootPosition(402.f, 380.f);

    static const std::int32_t BackgroundDepth = -20;
    static const std::int32_t MenuDepth = 0;
    static const std::int32_t ButtonDepth = 1;
    static const std::int32_t TextDepth = 5;
}