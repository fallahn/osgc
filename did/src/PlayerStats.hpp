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

struct PlayerStats final
{
    void reset()
    {
        shotsFired = 0;
        foesSlain = 0;
        livesLost = 0;
        roundXP = 0;
        totalXP = 0;
        currentLevel = 1;
    }
    std::uint16_t shotsFired = 0;
    std::uint16_t foesSlain = 0;
    std::uint16_t livesLost = 0;
    std::uint16_t roundXP = 0;
    std::size_t totalXP = 0;
    std::size_t currentLevel = 1;
};