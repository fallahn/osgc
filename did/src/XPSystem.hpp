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

#include <cstdint>
#include <cstddef>

namespace XP
{
    //base scores which are multiplied during the game
    //before being awarded
    static constexpr std::uint32_t TreasureScore = 1;
    static constexpr std::uint32_t SkellyScore = 10;
    static constexpr std::uint32_t PlayerScore = 3;
    //life bonus is awarded for each life under 5 not lost
    //eg 0 lives lost is 5x this score, 5 lives lost is 0
    static constexpr std::uint32_t LifeBonus = 5;
    static constexpr std::uint32_t BotScore = 1;


    //calculates the player level from current XP
    std::uint32_t calcLevel(std::uint32_t xp);

    //calculates how much you actually earn from beating
    //a player depending on the difference in levels
    std::uint32_t calcPlayerScore(std::uint32_t difference);
}