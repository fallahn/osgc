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
    enum
    {
        PlayerTop = 0x1,
        PlayerSide = 0x2,
        BatteryMeter = 0x4,
        HealthMeter = 0x8,
        LifeMeter = 0x10,
        AmmoMeter = 0x20,
        BackgroundTop = 0x40,
        ScoreText = 0x80,
        HumanCount = 0x100,
        AlienCount = 0x200,
        BatteryWarningText = 0x400
    };

    enum Menu
    {
        RootNode = 0x1,
        Starfield = 0x2,
        Help = 0x4,
        TextCrawl = 0x8,
        DifficultySelect = 0x10,
        ScoreMenu = 0x20
    };
}
