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
        PlayerAvatar    = 0x01,
        PlayerTwo       = 0x02,
        NetInterpolator = 0x04,
        Weapon          = 0x08,
        LocalPlayer     = 0x10,
        HealthBar       = 0x20,
        NameTag         = 0x40,
        Bird            = 0x80,
        DaySound        = 0x100,
        NightSound      = 0x200,
        LoopedSound     = 0x400,
        Parrot          = 0x800,
        Rain            = 0x1000,
        RainSound       = 0x2000,
        InventoryHolder = 0x4000,
        DebugLabel      = 0x8000,
        ShipLight       = 0x10000,
        BasicShadow     = 0x20000,
        Removable       = 0x400000,
        WindSound       = 0x800000,
        Music           = 0x1000000
    };
}