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
        Fluid = 0x2,
        Spikes = 0x4,
        Checkpoint = 0x8,
        Enemy = 0x10,
        Exit = 0x20,
        Collectible = 0x40,

        Player = 0x80,
        Foot = 0x100,
        LeftHand = 0x200,
        RightHand = 0x400,

        Text = 0x800,
        Dialogue = 0x1000
    }type = Solid;

    //these are the types this shape collides with
    std::uint64_t collisionFlags = Player | Foot | LeftHand | RightHand;

    enum Shape
    {
        Rectangle, Polygon, Segment
    }shape = Rectangle;

    sf::FloatRect aabb;

    //used for map objects with properties such as NPC/Collectible type
    //or index into speech text file array
    std::int32_t ID = -1; 
};

struct CollisionBody final
{
    std::array<CollisionShape, 4u> shapes;
    std::size_t shapeCount = 0;
    std::uint16_t collisionFlags = 0; //types on this body which currently have collisions
};

struct Manifold final
{
    sf::Vector2f normal;
    float penetration = 0.f;
};

namespace CollisionGroup
{
    static const std::uint64_t PlayerFlags = 
        CollisionShape::Fluid | CollisionShape::Solid | CollisionShape::Spikes | 
        CollisionShape::Collectible | CollisionShape::Checkpoint | CollisionShape::Enemy |
        CollisionShape::Exit | CollisionShape::Dialogue;

    static const std::uint64_t StarFlags =
        CollisionShape::Solid | CollisionShape::Fluid | CollisionShape::Spikes |
        CollisionShape::Enemy | CollisionShape::Text;
}

std::optional<Manifold> intersectsAABB(sf::FloatRect, sf::FloatRect);

std::optional<Manifold> intersectsSAT(xy::Entity, xy::Entity);