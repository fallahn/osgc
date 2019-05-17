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
#include <xyginext/util/Vector.hpp>

#include <SFML/System/Vector2.hpp>

#include <utility>
#include <vector>
#include <optional>
#include <limits>

using Segment = std::pair<sf::Vector2f, sf::Vector2f>;

namespace CollisionFlags
{
    enum
    {
        Asteroid = 0x1,
        Static = 0x2,
        Vehicle = 0x4
    };
}

struct Manifold final
{
    sf::Vector2f normal;
    float penetration = -std::numeric_limits<float>::max();
};

struct CollisionObject final
{
    enum Type
    {
        Fence, Jump, Collision, KillZone, Space, Waypoint, Vehicle,

        Count
    }type = Collision;

    //we'll do all these in local space for now, but a potential
    //optimisation is to move static objects to world space so
    //that collision doesn't have to needlessly transform the
    //points every test...
    std::vector<sf::Vector2f> vertices; //clockwise winding
    std::vector<sf::Vector2f> normals; //direction of the face created by the current vert and next clockwise vert

    void applyVertices(const std::vector<sf::Vector2f>& points);
};

// SAT test functions from http://blog.marcher.co/sat1/ 
sf::Vector2f getSupportPoint(xy::Entity, sf::Vector2f);

Manifold minimumPenetration(xy::Entity, xy::Entity);

std::optional<Manifold> intersects(xy::Entity, xy::Entity);

//from Andre LaMothe's black art of game programming
static inline std::optional<Manifold> intersects(const Segment& segOne, const Segment& segTwo)
{
    //in the case of vehicle->static collision we assume segOne v.centre->v.point
    //segTwo static object segment TODO check the winding of segments, they ought to be clockwise

    sf::Vector2f dirOne = segOne.second - segOne.first;
    sf::Vector2f dirTwo = segTwo.second - segTwo.first;

    float s = (-dirOne.y * (segOne.first.x - segTwo.first.x) + dirOne.x * (segOne.first.y - segTwo.first.y)) / (-dirTwo.x * dirOne.y + dirOne.x * dirTwo.y);
    float t = (dirTwo.x * (segOne.first.y - segTwo.first.y) - dirTwo.y * (segOne.first.x - segTwo.first.x)) / (-dirTwo.x * dirOne.y + dirOne.x * dirTwo.y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        //collision detected
        auto lineSide = [&segTwo](sf::Vector2f point)->float
        {
            const auto& [a, b] = segTwo;
            auto sign = ((b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - b.x));
            return sign > 0 ? 1.f : -1.f;
        };

        Manifold retVal;
        //use line side of second point to find normal direction
        retVal.normal = xy::Util::Vector::normalise({ dirTwo.y, -dirTwo.x }); //anticlockwise 90
        retVal.normal *= lineSide(segOne.second);

        //penetration is normal dot segment - intersection
        sf::Vector2f intersection;
        intersection.x = segOne.first.x + (t * dirOne.x);
        intersection.y = segOne.first.y + (t * dirOne.y);

        retVal.penetration = xy::Util::Vector::dot(retVal.normal, segOne.second - intersection);

        return std::optional<Manifold>{retVal};
    }

    return std::nullopt;
}