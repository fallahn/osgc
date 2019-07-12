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

#include <xyginext/ecs/components/Sprite.hpp>

#include <array>

namespace ShaderID
{
    enum
    {
        TileMap,
        PixelTransition,
        NoiseTransition
    };
}

namespace TextureID
{
    namespace Menu
    {
        enum
        {
            Background,

            Count
        };
    }

    namespace Game
    {
        enum
        {
            Background,
            UIBackground,

            Count
        };
    }
}

namespace FontID
{
    enum
    {
        Menu,


        Count 
    };

    static const std::string GearBoyFont("assets/fonts/gameboy.ttf");
}

namespace ParticleID
{
    enum
    {
        Shield,
        Checkpoint,

        Count
    };
}

namespace SpriteID
{
    namespace GearBoy
    {
        enum
        {
            Player,
            Star,
            SmokePuff,
            Squidger,
            Lava,
            Water,
            Checkpoint,

            Coin,
            Shield,
            Ammo,
            ShieldAvatar,

            Crawler,
            Bird,
            Walker,
            Bomb,

            Count
        };
    }
}

template <std::size_t size>
using SpriteArray = std::array<xy::Sprite, size>;