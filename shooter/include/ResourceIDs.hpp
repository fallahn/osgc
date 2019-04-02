/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#pragma once

#include <array>

namespace FontID
{
    enum
    {
        HandDrawn,
        Count
    };

    static std::array<std::size_t, FontID::Count> handles;
}