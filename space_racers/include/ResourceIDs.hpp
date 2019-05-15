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
#include <array>

namespace FontID
{
    enum
    {
        Default,

        Count
    };

    static std::array<std::size_t, Count> handles = {};
}

namespace TextureID
{
    enum
    {
        MainMenu,

        Temp01, Temp02,

        Count
    };

    static std::array<std::size_t, Count> handles = {};
}

#include <xyginext/ecs/components/Sprite.hpp>
namespace SpriteID
{
    enum
    {
        TimeTrialButton,
        LocalButton,
        NetButton,
        OptionsButton,
        QuitButton,

        Count
    };

    static std::array<xy::Sprite, Count> sprites;
}