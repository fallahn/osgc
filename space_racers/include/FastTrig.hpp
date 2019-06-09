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

class ft
{
public:

    static float sin(float n)
    {
        return instance().getSin(n);
    }

    static float cos(float n)
    {
        return instance().getCos(n);
    }

    ft(const ft&) = delete;
    ft(ft&&) = delete;

    const ft& operator = (const ft&) = delete;
    ft& operator = (ft&&) = delete;

private:

    ft() { init(); }

    static ft& instance()
    {
        static ft instance;

        return instance;
    }

    const std::int32_t MAX_CIRCLE_ANGLE = 512;
    const std::int32_t HALF_MAX_CIRCLE_ANGLE = (MAX_CIRCLE_ANGLE / 2);
    const std::int32_t QUARTER_MAX_CIRCLE_ANGLE = (MAX_CIRCLE_ANGLE / 4);
    const std::int32_t MASK_MAX_CIRCLE_ANGLE = (MAX_CIRCLE_ANGLE - 1);
    const float PI = 3.14159265358979323846f;

    std::vector<float> m_table;

    //copied from NVidia web site
    inline void FloatToInt(int* int_pointer, float f) const
    {
        //__asm  fld  f
        //__asm  mov  edx, int_pointer
        //__asm  FRNDINT
        //__asm  fistp dword ptr[edx];

        //I'm not going to pretend to understand it
        //so I'll do it this way.
        *int_pointer = static_cast<int>(f);
    }

    void init()
    {
        if (m_table.empty())
        {
            for (auto i = 0; i < MAX_CIRCLE_ANGLE; i++)
            {
                m_table.emplace_back(static_cast<float>(std::sin(static_cast<double>(i * PI / HALF_MAX_CIRCLE_ANGLE))));
            }
        }
    }

    float getCos(float n) const
    {
        float f = n * HALF_MAX_CIRCLE_ANGLE / PI;
        int i;
        FloatToInt(&i, f);
        if (i < 0)
        {
            return m_table[((-i) + QUARTER_MAX_CIRCLE_ANGLE) & MASK_MAX_CIRCLE_ANGLE];
        }
        else
        {
            return m_table[(i + QUARTER_MAX_CIRCLE_ANGLE) & MASK_MAX_CIRCLE_ANGLE];
        }
    }

    float getSin(float n) const
    {
        float f = n * HALF_MAX_CIRCLE_ANGLE / PI;
        int i;
        FloatToInt(&i, f);
        if (i < 0)
        {
            return m_table[(-((-i) & MASK_MAX_CIRCLE_ANGLE)) + MAX_CIRCLE_ANGLE];
        }
        else
        {
            return m_table[i & MASK_MAX_CIRCLE_ANGLE];
        }
    }
};