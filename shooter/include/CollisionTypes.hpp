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

#include <SFML/Graphics/Rect.hpp>

struct CollisionBox final
{
    sf::FloatRect worldBounds;
    enum Type
    {
        Building,
        Structure,
        NPC,
        Fire,
        Water,
        Ammo,
        Battery
    }type = Structure;

    enum Filter
    {
        Solid = 0x1,
        NoDecal = 0x2,
        Navigation = 0x4,
        Collectible = 0x8,
        Alien = 0x10,
        Explosion = 0x20,
        Human = 0x40
    };
    std::uint64_t filter = Solid;

    float height = 0.f; //subtracted from drop depth to decide if drone collides
};