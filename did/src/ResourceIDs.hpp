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

#include <array>

namespace ShaderID
{
    enum
    {
        SpriteShader,
        SpriteShaderCulled,
        SpriteShaderUnlit,
        SpriteShaderUnlitTextured,
        PlaneShader,
        LandShader,
        SkyShader,
        SeaShader,
        MoonShader,
        SunsetShader,
        ShadowShader
    };
}

namespace SpriteID
{
    enum
    {
        PlayerOne,
        PlayerTwo,
        PlayerThree,
        PlayerFour,
        Ghost,
        Crab,
        Skeleton,
        Treasure,
        Lantern,
        WetPatch,
        WeaponRodney,
        WeaponJean,
        Ammo,
        Coin,
        Rings,
        Fire,
        Boat,
        Sail,
        Ships,
        ShipLights,
        Dog,
        BarrelOne,
        BarrelTwo,
        Food,
        Bees,
        Beehive,
        Decoy,
        DecoyItem,
        Flare,
        FlareItem,
        SkullShield,
        SkullItem,
        MineItem,
        SmokePuff,
        Impact,
        WaterSplash,
        PlayerPuff,
        Count
    };
}

namespace AnimationID
{
    enum
    {
        IdleUp,
        IdleDown,
        IdleLeft,
        IdleRight,
        WalkLeft,
        WalkRight,
        WalkUp,
        WalkDown,
        Spawn,
        Die,
        Count
    };

    enum Barrel
    {
        Idle,
        Break,
        Explode
    };
}
using AnimationMap = std::array<std::size_t, AnimationID::Count>;

namespace SpriteDepth
{
    enum
    {
        Sky = -500,
        Sun = -400,
        Island = -300
    };
}