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
        PlayerAvatar = 1,
        PlayerTwo = 2,
        NetInterpolator = 4,
        Weapon = 8,
        LocalPlayer = 16,
        HealthBar = 32,
        NameTag = 64,
        Bird = 128,
        DaySound = 256,
        NightSound = 512,
        LoopedSound = 1024,
        Parrot = 2048,
        Rain = 4096,
        RainSound = 8192,
        InventoryHolder = 16384,
        DebugLabel = 32768,
        ShipLight = 65536,
        BasicShadow = 0x20000,
        Removable = 0x400000
    };
}