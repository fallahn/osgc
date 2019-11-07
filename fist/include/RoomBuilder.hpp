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

#include "GameConst.hpp"

#include <SFML/Graphics/Vertex.hpp>

#include <vector>

enum Direction
{
    Nort, Eas, Sout, Wes //these names clash with something windowsy 0.o
};

static const sf::FloatRect NorthUV(540.f, 0.f, 960.f, 540.f);
static const sf::FloatRect EastUV(2040.f, 540.f, -540.f, 960.f);
static const sf::FloatRect SouthUV(1500.f, 2040.f, -960.f, -540.f);
static const sf::FloatRect WestUV(0.f, 1500.f, 540.f, -960.f);
static const sf::FloatRect FloorUV(540.f, 540.f, 960.f, 960.f);
static const sf::FloatRect CeilingUV(2040.f, 540.f, 960.f, 960.f);

static void addFloor(std::vector<sf::Vertex>& verts)
{
    const float Width = GameConst::RoomWidth + GameConst::RoomPadding;
    std::vector<sf::Vertex> temp;

    sf::Vertex vert;
    vert.color.a = 0;
    //put normal data in colour rgb
    vert.color.r = 127;
    vert.color.g = 127;
    vert.color.b = 255;

    vert.position = { -Width, -Width };
    vert.position /= 2.f;

    vert.texCoords = { FloorUV.left, FloorUV.top };
    temp.push_back(vert);

    vert.position.x += Width;
    vert.texCoords.x += FloorUV.width;
    temp.push_back(vert);

    vert.position.y += Width;
    vert.texCoords.y += FloorUV.height;
    temp.push_back(vert);

    vert.position.x -= Width;
    vert.texCoords.x -= FloorUV.width;
    temp.push_back(vert);

    std::reverse(temp.begin(), temp.end());
    verts.insert(verts.end(), temp.begin(), temp.end());
}

static void addCeiling(std::vector<sf::Vertex>& verts)
{
    const float Width = GameConst::RoomWidth + GameConst::RoomPadding;

    //using this to reverse the winding because I cba to manually rearrange the code
    std::vector<sf::Vertex> temp;

    sf::Vertex vert;
    vert.color.a = 255;
    //put normal data in colour rgb
    vert.color.r = 127;
    vert.color.g = 127;
    vert.color.b = 0;

    vert.position = { Width, -Width };
    vert.position /= 2.f;

    vert.texCoords = { CeilingUV.left, CeilingUV.top };
    temp.push_back(vert);

    vert.position.x -= Width;
    vert.texCoords.x += CeilingUV.width;
    temp.push_back(vert);

    vert.position.y += Width;
    vert.texCoords.y += CeilingUV.height;
    temp.push_back(vert);

    vert.position.x += Width;
    vert.texCoords.x -= CeilingUV.width;
    temp.push_back(vert);

    std::reverse(temp.begin(), temp.end());
    verts.insert(verts.end(), temp.begin(), temp.end());
}

static void addWall(std::vector<sf::Vertex>& verts, std::int32_t dir)
{
    const float Width = GameConst::RoomWidth + GameConst::RoomPadding;

    std::vector<sf::Vertex> temp;
    switch (dir)
    {
    default: return;
    case Nort:
    {
        sf::Vertex vert;
        vert.color.a = 255;
        //put normal data into colour property
        vert.color.r = 127;
        vert.color.g = 255;
        vert.color.b = 127;

        vert.position = { -Width, -Width };
        vert.position /= 2.f;

        vert.texCoords = { NorthUV.left, NorthUV.top };
        temp.push_back(vert);

        vert.position.x += Width;
        vert.texCoords.x += NorthUV.width;
        temp.push_back(vert);

        vert.color.a = 0;
        vert.texCoords.y += NorthUV.height;
        temp.push_back(vert);

        vert.position.x -= Width;
        vert.texCoords.x -= NorthUV.width;
        temp.push_back(vert);
    }
    break;
    case Sout:
    {
        sf::Vertex vert;
        vert.color.a = 255;
        //put normal data into colour property
        vert.color.r = 127;
        vert.color.g = 0;
        vert.color.b = 127;

        vert.position = { Width, Width };
        vert.position /= 2.f;

        vert.texCoords = { SouthUV.left, SouthUV.top };
        temp.push_back(vert);

        vert.position.x -= Width;
        vert.texCoords.x += SouthUV.width;
        temp.push_back(vert);

        vert.color.a = 0;
        vert.texCoords.y += SouthUV.height;
        temp.push_back(vert);

        vert.position.x += Width;
        vert.texCoords.x -= SouthUV.width;
        temp.push_back(vert);
    }
    break;
    case Eas:
    {
        sf::Vertex vert;
        vert.color.a = 255;
        //put normal data into colour property
        vert.color.r = 0;
        vert.color.g = 127;
        vert.color.b = 127;

        vert.position = { Width, -Width };
        vert.position /= 2.f;

        vert.texCoords = { EastUV.left, EastUV.top };
        temp.push_back(vert);

        vert.position.y += Width;
        vert.texCoords.y += EastUV.height;
        temp.push_back(vert);

        vert.color.a = 0;
        vert.texCoords.x += EastUV.width;
        temp.push_back(vert);

        vert.position.y -= Width;
        vert.texCoords.y -= EastUV.height;
        temp.push_back(vert);
    }
    break;
    case Wes:
    {
        sf::Vertex vert;
        vert.color.a = 255;
        //put normal data into colour property
        vert.color.r = 255;
        vert.color.g = 127;
        vert.color.b = 127;

        vert.position = { -Width, Width };
        vert.position /= 2.f;

        vert.texCoords = { WestUV.left, WestUV.top };
        temp.push_back(vert);

        vert.position.y -= Width;
        vert.texCoords.y += WestUV.height;
        temp.push_back(vert);

        vert.color.a = 0;
        vert.texCoords.x += WestUV.width;
        temp.push_back(vert);

        vert.position.y += Width;
        vert.texCoords.y -= WestUV.height;
        temp.push_back(vert);
    }
    break;
    }

    std::reverse(temp.begin(), temp.end());
    verts.insert(verts.end(), temp.begin(), temp.end());
}