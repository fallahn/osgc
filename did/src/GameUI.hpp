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

namespace UI
{
    namespace Colour
    {
        static const sf::Color RedText(240, 20, 12);
        static const sf::Color ScoreText(210, 125, 44);
        static const sf::Color MessageText(186, 17, 17);
    }

    namespace CommandID
    {
        enum
        {
            WeaponSlot = 0x1,
            Treasure = 0x2,
            Ammo = 0x4,
            Curtain = 0x8,
            Health = 0x10,
            Avatar = 0x20,
            InfoBar = 0x40,
            RoundEndMenu = 0x80,
            ServerMessage = 0x100,
            CarryIconOne = 0x200,
            CarryIconTwo = 0x400,
            CarryIconThree = 0x800,
            CarryIconFour = 0x1000,
            TreasureIcon = 0x2000,
            DeadIconOne = 0x4000,
            DeadIconTwo = 0x8000,
            DeadIconThree = 0x10000,
            DeadIconFour = 0x200000,
            MiniMap = 0x400000,
            TreasureScore = 0x800000,
            TrophyOne = 0x1000000,
            TrophyTwo = 0x2000000,
            TrophyThree = 0x4000000,
            TrophyFour = 0x8000000,
            RoundTimer = 0x10000000
        };
    }

    namespace SpriteID
    {
        enum
        {
            PlayerOneIcon,
            PlayerTwoIcon,
            PlayerThreeIcon,
            PlayerFourIcon,
            Bot,
            Count
        };
    }

    static const sf::Vector2f RoundScreenOffPosition(xy::DefaultSceneSize.x + 64.f, (xy::DefaultSceneSize.y - 768.f) / 2.f);
    static const sf::Vector2f RoundScreenOnPosition((xy::DefaultSceneSize.x - 1664.f) / 2.f, ((xy::DefaultSceneSize.y - 768.f) / 2.f) + 32.f);
    static const sf::Vector2f TreasureScorePosition(1664.f / 2.f, 80.f); //relative to scoreboard
}