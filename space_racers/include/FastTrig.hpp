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

//the following code is based on http://www.flipcode.com/archives/Fast_Trigonometry_Functions_Using_Lookup_Tables.shtml

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

namespace ft
{
    static const std::int32_t MAX_CIRCLE_ANGLE = 512;
    static const std::int32_t HALF_MAX_CIRCLE_ANGLE = (MAX_CIRCLE_ANGLE / 2);
    static const std::int32_t QUARTER_MAX_CIRCLE_ANGLE = (MAX_CIRCLE_ANGLE / 4);
    static const std::int32_t MASK_MAX_CIRCLE_ANGLE = (MAX_CIRCLE_ANGLE - 1);
    static const float PI = 3.14159265358979323846f;

    static std::vector<float> table;

    static inline void init()
    {
        if (table.empty())
        {
            for (auto i = 0; i < MAX_CIRCLE_ANGLE; i++)
            {
                table.emplace_back(static_cast<float>(std::sin(static_cast<double>(i * PI / HALF_MAX_CIRCLE_ANGLE))));
            }
        }
    }

    //copied from NVidia web site
    static inline void FloatToInt(int* int_pointer, float f)
    {
        //__asm  fld  f
        //__asm  mov  edx, int_pointer
        //__asm  FRNDINT
        //__asm  fistp dword ptr[edx];

        //I'm not going to pretend to understand it
        //so I'll do it this way.
        *int_pointer = static_cast<int>(f);
    }

    static inline float cos(float n)
    {
        float f = n * HALF_MAX_CIRCLE_ANGLE / PI;
        int i;
        FloatToInt(&i, f);
        if (i < 0)
        {
            return table[((-i) + QUARTER_MAX_CIRCLE_ANGLE) & MASK_MAX_CIRCLE_ANGLE];
        }
        else
        {
            return table[(i + QUARTER_MAX_CIRCLE_ANGLE) & MASK_MAX_CIRCLE_ANGLE];
        }
    }

    static inline float sin(float n)
    {
        float f = n * HALF_MAX_CIRCLE_ANGLE / PI;
        int i;
        FloatToInt(&i, f);
        if (i < 0)
        {
            return table[(-((-i) & MASK_MAX_CIRCLE_ANGLE)) + MAX_CIRCLE_ANGLE];
        }
        else
        {
            return table[i & MASK_MAX_CIRCLE_ANGLE];
        }
    }
}