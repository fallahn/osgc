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

#include <SFML/Graphics/Rect.hpp>

#include <array>
#include <optional>

struct CollisionShape final
{
    //also used as flags for collision filtering
    enum Type
    {
        Solid = 0x1,
        Water = 0x2,
        Player = 0x4,
        Sensor = 0x8
    }type = Solid;

    //these are the types this shape collides with
    std::uint16_t collisionFlags = Player | Sensor;

    enum Shape
    {
        Rectangle, Polygon, Segment
    }shape = Rectangle;

    sf::FloatRect aabb;
};

struct CollisionBody final
{
    std::array<CollisionShape, 4u> shapes;
    std::size_t shapeCount = 0;
};

struct Manifold final
{
    sf::Vector2f normal;
    float penetration = 0.f;
};

std::optional<Manifold> intersectsAABB(sf::FloatRect, sf::FloatRect);

std::optional<Manifold> intersectsSAT(xy::Entity, xy::Entity);