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

struct Actor final
{
    //these are ordered by priority in which
    //bots should target them
    enum ID
    {
        None,
        PlayerOne,
        PlayerTwo,
        PlayerThree,
        PlayerFour, //Don't alter the player enum values!
        Skeleton,

        Boat,
        Treasure,
        Coin,
        Barrel,
        Ammo,
        Food,
        Lantern,
        FlareItem,
        DecoyItem,
        SkullItem,
        MineItem,
        Crab,

        TreasureSpawn,
        AmmoSpawn,
        CoinSpawn,
        MineSpawn,
        Weapon,
        Fire,
        Parrot,
        Explosion,
        DirtSpray,
        Lightning,
        Bees,
        SkullShield,
        SmokePuff,
        Flare,
        Decoy,
        Magnet,
        Impact
    }id = None;

    std::uint32_t entityID = 0;
    std::uint32_t serverID = 0;
    std::int8_t direction = 0;
    bool carryingItem = false; //not sync'd with server, updated by client animation
    bool nonStatic = true; //server decides based on this if updates should be sent

    bool counted = true;
};
