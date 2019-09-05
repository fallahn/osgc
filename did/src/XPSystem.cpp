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

#include "XPSystem.hpp"

#include <algorithm>

/*
the gap between each level increases by 100 eg:
Level   XP      Difference
1       0       -
2       100     100
3       300     200
4       600     300
5       1000    400
*/

std::size_t XP::calcLevel(std::size_t xpAmount)
{
    std::size_t level = 0;
    std::size_t difference = 100;
    std::size_t currentXP = 0;

    while (currentXP < xpAmount)
    {
        currentXP += difference;
        difference += 100;
        level++;
    }
    return level;
}

/*
players are awarded a bigger score when beating someone much
higher ranked than them. Someone less the 5 levels below will
award 0 XP
*/
std::size_t XP::calcPlayerScore(std::uint64_t difference)
{
    difference = std::max(std::uint64_t(-5), difference);
    difference += 5;
    return difference * PlayerScore;
}