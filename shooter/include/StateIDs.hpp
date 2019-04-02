/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#pragma once

#include <xyginext/resources/ResourceHandler.hpp>

namespace StateID
{
    enum
    {
        MainMenu
    };
}

struct SharedData final
{
    xy::ResourceHandler resources;
};