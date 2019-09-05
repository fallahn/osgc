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

#include <xyginext/ecs/Entity.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <array>
#include <cmath>

namespace ManifoldID
{
    enum
    {
        None = -1,
        Terrain = 0,
        Collectible,
        Player,
        Skeleton,
        Boat,
        SpawnArea,
        Barrel,
        Explosion,
        SkullShield
    };
}

struct Manifold final
{
    sf::Vector2f normal;
    float penetration = 0.f;
    std::int16_t ID = ManifoldID::None;
    xy::Entity otherEntity;
};

struct Circle final
{
    sf::Vector2f centre;
    float radius = 0.f;
    bool contains(sf::Vector2f point)
    {
        auto dist = centre - point;
        auto len2 = (dist.x * dist.x) + (dist.y * dist.y);
        return len2 < (radius * radius);
    }

    bool intersects(const Circle& other)
    {
        auto dist = centre - other.centre;
        auto minDist = (radius * radius) + (other.radius) * (other.radius);
        auto len2 = (dist.x * dist.x) + (dist.y * dist.y);
        return len2 < minDist;
    }

    bool intersects(const Circle& other, Manifold& output)
    {
        auto dist = centre - other.centre;
        auto minDist = (radius * radius) + (other.radius) * (other.radius);
        auto len2 = (dist.x * dist.x) + (dist.y * dist.y);

        output.normal = std::sqrt(len2) * dist;
        output.penetration = std::sqrt(minDist - len2);

        return len2 < minDist;
    }
};

//https://github.com/mattdesl/line-circle-collision/blob/master/index.js 

struct CollisionComponent final
{
    static constexpr std::size_t MaxManifolds = 8;
    sf::FloatRect bounds;
    std::array<Manifold, MaxManifolds> manifolds;
    std::size_t manifoldCount = 0;
    std::int16_t ID = ManifoldID::None;
    
    //normalised water value. Calculated by the
    //number of surrounding solid tiles and the
    //averaged distance to them.
    float water = 0.f;

    //not everything needs terrain collision
    bool collidesTerrain = true;
    bool collidesBoat = false;
};
