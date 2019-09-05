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

#include <random>

namespace Server
{
    static std::mt19937 rndEngine;

    static inline float getRandomFloat(float begin, float end)
    {
        //XY_ASSERT(begin < end, "first value is not less than last value");
        std::uniform_real_distribution<float> dist(begin, end);
        return dist(rndEngine);
    }

    static inline int getRandomInt(int begin, int end)
    {
        //XY_ASSERT(begin < end, "first value is not less than last value");
        std::uniform_int_distribution<int> dist(begin, end);
        return dist(rndEngine);
    }
}