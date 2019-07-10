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

namespace CommandID
{
    namespace Menu
    {
        enum
        {
            Cursor = 0x1,
            Message = 0x2
        };
    }

    namespace World
    {
        enum
        {
            DebugItem = 0x1,
            CheckPoint = 0x2,
            ShieldParticle = 0x4
        };
    }
    namespace UI
    {
        enum
        {
            CoinText = 0x1,
            ScoreText = 0x2,
            LivesText = 0x4,
            TimeText = 0x8
        };
    }
}
