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

#include <cstdint>

namespace Menu
{
    static const sf::Vector2f TitlePosition(120.f, 80.f);
    static const std::uint32_t TitleCharSize = 128;

    static const sf::Vector2f ItemFirstPosition(120.f, 480.f);
    static const float ItemVerticalSpacing = 88.f;
    static const std::uint32_t ItemCharSize = 64;
    static const sf::Color TextColour = sf::Color::Yellow;

    static const sf::Vector2f DifficultyShownPosition(xy::DefaultSceneSize / 2.f);
    static const float PlanetHiddenPosition = xy::DefaultSceneSize.y;
    static const float StarfieldUpPosition = 0.f;
    static const sf::Vector2f HelpShownPosition(20.f, 20.f);
    static const float StarfieldDownPosition = -xy::DefaultSceneSize.y / 2.f;
    static const float HelpHiddenPosition = 1100.f;
    static const float PlanetShownPosition = 0.f;

    static const std::int32_t BackgroundRenderDepth = -10;
    static const std::int32_t MenuRenderDepth = 10;
    static const std::int32_t TextRenderDepth = 1;

    static const std::string AppName("drone_drop");
    static const std::string ConfigName("keybinds.cfg");
    static const std::string ScoreEasyName("easy.sco");
    static const std::string ScoreMedName("med.sco");
    static const std::string ScoreHardName("hard.sco");
}