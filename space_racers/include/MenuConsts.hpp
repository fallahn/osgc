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
    static const sf::Vector2f TimeTrialMenuPosition(0.f, xy::DefaultSceneSize.y);
    static const sf::Vector2f LocalPlayMenuPosition(0.f, -xy::DefaultSceneSize.y);
    static const sf::Vector2f ItemRootPosition(402.f, 380.f);

    static const sf::FloatRect VehicleSelectArea(0.f, 0.f, 517.f, 268.f);// (22.f, 50.f, 471.f, 186.f);
    static const sf::FloatRect VehicleButtonArea(22.f, 50.f, 471.f, 186.f);
    static const sf::Vector2f NavLeftPosition(180.f, 980.f);
    static const sf::Vector2f NavRightPosition(xy::DefaultSceneSize.x - NavLeftPosition.x, NavLeftPosition.y);

    static const sf::Vector2f ThumbnailPosition(46.f, 56.f);
    static const sf::Vector2f PrevTrackPosition(80.f, 332.f);
    static const sf::Vector2f NextTrackPosition(262.f, 332.f);
    static const sf::Vector2f TrackButtonSize(66.f, 24.f);

    static const sf::Vector2f LapPrevPosition(18.f, 18.f);
    static const sf::Vector2f LapNextPosition(108.f, 18.f);
    static const sf::Vector2f LapButtonSize(32.f, 32.f);

    static const sf::Vector2f ModeSelectDialPosition(93.5f, 97.5f);
    static const float ModeSelectSpeed = 6.f;

    static const sf::Vector2f TogglePosition(280.f, 238.f);
    static const sf::Vector2f LightbarPosition(30.f, 248.f);
    static const sf::Vector2f LapDigitPosition(51.f, 13.f);

    static const std::int32_t BackgroundDepth = -20;
    static const std::int32_t MenuDepth = 0;
    static const std::int32_t ButtonDepth = 1;
    static const std::int32_t TextDepth = 5;
}