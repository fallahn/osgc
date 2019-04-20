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

#include <SFML/Graphics/Rect.hpp>

#include <cstdint>

namespace ConstVal
{
    static const std::int32_t BackgroundDepth = -100;
    static const sf::Vector2f BackgroundPosition(xy::DefaultSceneSize.x * 2.f, 0.f);
    static const float DroneHeight = 100.f;

    static const sf::FloatRect SmallViewPort(0.1f, 0.3f, 0.8f, 0.7f); //this should be relative to active large viewport
    static const sf::Vector2f SmallViewSize(xy::DefaultSceneSize.x * SmallViewPort.width, xy::DefaultSceneSize.y * SmallViewPort.height);

    static const sf::FloatRect MapArea(0.f, 0.f, 2880.f, 3840.f);
}