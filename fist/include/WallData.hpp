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

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>

struct WallData final
{
    bool passable = true;
    sf::Vector2f targetPoint;
};

struct RoomData final
{
    //TODO list of visible rooms for rendering
    sf::Vector3f skyColour = { 1.f,1.f,1.f };
    sf::Vector3f roomColour = { 1.f,1.f,1.f };
    std::int32_t id = -1; //used to sort the array
    bool hasCeiling = false;
};

struct MapData final
{
    std::vector<RoomData> roomData;
    std::size_t prevRoomIndex = 0;
    //current room index is in state shared data
    //as the map editor needs to knoe what it is

    //interpolated values during cam movements
    sf::Vector3f currentSkyColour;
    sf::Vector3f currentRoomColour;

    float skyAmount = 1.f;
    float roomAmount = 0.f;
    static constexpr float MinSkyAmount = 0.1f;
};