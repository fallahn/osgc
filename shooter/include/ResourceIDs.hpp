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

#include <array>

namespace FontID
{
    enum
    {
        CGA,
        Count
    };

    extern std::array<std::size_t, FontID::Count> handles;
}

namespace TextureID
{
    enum
    {
        Clouds,
        Sidebar,
        Noise,
        MenuBackground,
        HowToPlay,
        DifficultySelect,
        HighScores,

        Count
    };

    extern std::array<std::size_t, TextureID::Count> handles;
}

namespace ShaderID
{
    enum
    {
        Cloud,
        Noise,
        Water
    };
}

#include <xyginext/ecs/components/Sprite.hpp>
namespace SpriteID
{
    enum
    {
        Explosion,
        ExplosionIcon,
        TankIcon,
        HillLeftWide,
        HillLeftNarrow,
        HillCentre,
        HillRightNarrow,
        HillRightWide,
        TreeIcon,
        BushIcon,
        BuildingLeft,
        BuildingCentre,
        BuildingRight,
        Drone,
        ScorpionIcon,
        AmmoIcon,
        BatteryIcon,
        BeetleIcon,

        Crosshair,
        AmmoTop,
        BatteryTop,
        HealthBar,
        BatteryBar,

        Crater01,
        Crater02,
        Crater03,
        Crater04,

        Human,
        Beetle,
        Scorpion,

        HumanBody,
        BeetleBody,
        ScorpionBody,

        Science01,
        Science02,

        Count
    };
}
using SpriteArray = std::array<xy::Sprite, SpriteID::Count>;