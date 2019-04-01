/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#pragma once

#include <array>

#include <xyginext/ecs/components/Sprite.hpp>

namespace SpriteID
{
    enum
    {

        Count
    };

    static std::array<xy::Sprite, SpriteID::Count> sprites = {};
}

namespace FontID
{
    enum
    {
        MenuFont,
        Count
    };

    static std::array<std::size_t, Count> handles = {};
}