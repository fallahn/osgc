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

namespace QuadTreeFilter
{
    enum
    {
        Player = 0x1,
        WetPatch = 0x2,
        Collectable = 0x4,
        Skeleton = 0x8,
        Light = 0x10,
        Boat = 0x20,
        SpawnArea = 0x40,
        Treasure = 0x80,
        BotQuery = 0x100, //used in wide sweep by bot system to see if anything can be collected
        ParrotLauncher = 0x200,
        Barrel = 0x400,
        Explosion = 0x800,
        Decoy = 0x1000,
        DecoyItem = 0x2000, //this is the carriable version rather than deployed entity
        FlareItem = 0x4000,
        SkullItem = 0x8000,
        SkullShield = 0x10000,
        MineItem = 0x200000,

        Carriable = Light | Treasure | DecoyItem | FlareItem | SkullItem | MineItem | Boat, //boats aren't carrible but can have treasure taken from them
        Collidable = Player | Collectable | Skeleton | Boat | SpawnArea | ParrotLauncher | Barrel | Explosion | SkullShield
    };
}
