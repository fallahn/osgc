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
#include <xyginext/ecs/components/Transform.hpp>

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
        Vehicle = 0x4,


        All = Asteroid | Static | Vehicle
    };
}

static inline std::vector<sf::Vector2f> createCollisionCircle(float radius, sf::Vector2f origin, std::size_t pointCount = 8)
{
    std::vector<sf::Vector2f> retVal;
    const float step = xy::Util::Const::TAU / static_cast<float>(pointCount);

    for (auto i = 0.f; i < xy::Util::Const::TAU; i += step)
    {
        retVal.emplace_back(std::sin(i), std::cos(i));
        retVal.back() *= radius;
        retVal.back() += origin;
    }

    return retVal;
}

struct CollisionObject final
{
    enum Type
    {
        Collision, KillZone, Space, Waypoint, Jump, Fence, Vehicle, Roid,

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

struct Manifold final
{
    sf::Vector2f normal;
    float penetration = -std::numeric_limits<float>::max();
};

static inline bool contains(sf::Vector2f point, xy::Entity entity)
{
    const auto& tx = entity.getComponent<xy::Transform>().getTransform();
    const auto& points = entity.getComponent<CollisionObject>().vertices;

    //check if enough poly points
    if (points.size() < 3) return false;

    //else raycast through points - assuming given point is in world coords
    std::size_t i = 0;
    std::size_t j = 0;
    bool result = false;

    for (i = 0, j = points.size() - 1; i < points.size(); j = i++)
    {
        sf::Vector2f pointI = tx.transformPoint(points[i]);
        sf::Vector2f pointJ = tx.transformPoint(points[j]);

        if (((pointI.y > point.y) != (pointJ.y > point.y))
            && (point.x < (pointJ.x - pointI.x) * (point.y - pointI.y) / (pointJ.y - pointI.y) + pointI.x))
        {
            result = !result;
        }
    }
    return result;
}

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