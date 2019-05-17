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

#include "CollisionObject.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Vector.hpp>

void CollisionObject::applyVertices(const std::vector<sf::Vector2f>& points)
{
    //clockwise if > 0
    auto getWinding = [](sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3)->float
    {
        return p1.x* p2.y + p2.x * p3.y + p3.x * p1.y - p1.y * p2.x - p2.y * p3.x - p3.y * p1.x;
    };

    if (points.size() > 1)
    {
        bool clockwise = true;
        if (points.size() > 2)
        {
            clockwise = (getWinding(points[0], points[1], points[2]) > 0);
        }

        for (auto i = 0u; i < points.size() - 1; ++i)
        {
            //these are local space
            auto a = points[i];
            auto b = points[i + 1];

            if (clockwise)
            {
                vertices.emplace_back(a);
                sf::Vector2f normal = b - a;
                normals.emplace_back(xy::Util::Vector::normalise({ normal.y, -normal.x }));
                //LOG("Correct Winding", xy::Logger::Type::Info);
            }
            else
            {
                vertices.emplace_back(b);
                sf::Vector2f normal = a - b;
                normals.emplace_back(xy::Util::Vector::normalise({ normal.y, -normal.x }));
                //LOG("reversed winding", xy::Logger::Type::Info);
            }
        }
        //first / last closing vert (depending on winding)
        auto first = points.front();
        auto last = points.back();
        if (clockwise)
        {
            vertices.emplace_back(last.x, last.y);
            auto normal = first - last;
            normals.push_back(xy::Util::Vector::normalise({ normal.y, -normal.x }));
        }
        else
        {
            vertices.emplace_back(first.x, first.y);
            auto normal = last - first;
            normals.push_back(xy::Util::Vector::normalise({ normal.y, -normal.x }));
        }
    }
}

sf::Vector2f getSupportPoint(xy::Entity entity, sf::Vector2f axis)
{
    //this assumes the input axis is already in world coords

    auto tx = entity.getComponent<xy::Transform>().getWorldTransform();
    const auto& vertices = entity.getComponent<CollisionObject>().vertices;

    auto support = tx.transformPoint(vertices[0]);

    float maxProjection = xy::Util::Vector::dot(support, axis);

    for (auto i = 1u; i < vertices.size(); i++)
    {
        auto txPoint = tx.transformPoint(vertices[i]);
        float projection = xy::Util::Vector::dot(txPoint, axis);
        if (projection > maxProjection)
        {
            support = txPoint;
            maxProjection = projection;
        }
    }
    return support;
}

Manifold minimumPenetration(xy::Entity first, xy::Entity second)
{
    Manifold retVal;

    const auto& tx = first.getComponent<xy::Transform>();
    const auto& vertices = first.getComponent<CollisionObject>().vertices;
    const auto& normals = first.getComponent<CollisionObject>().normals;

    //end early if we ever observe separation
    for (auto i = 0u; retVal.penetration < 0 && i < normals.size(); i++) 
    {
        //only apply rotation to normal, remove any translation.
        auto normal = tx.getWorldTransform().transformPoint(normals[i]) - tx.getWorldPosition();
        auto support = getSupportPoint(second, -normal);

        auto vert = tx.getWorldTransform().transformPoint(vertices[i]);

        float projection = xy::Util::Vector::dot(normal, support - vert);
        if (projection > retVal.penetration) 
        {
            retVal.penetration = projection;
            retVal.normal = normal;
        }
    }
    return retVal;
}

std::optional<Manifold> intersects(xy::Entity a, xy::Entity b)
{
    auto manA = minimumPenetration(a, b);
    if (manA.penetration > 0)
    {
        return std::nullopt;
    }

    auto manB = minimumPenetration(b, a);
    if (manB.penetration > 0)
    {
        return std::nullopt;
    }

    return (manA.penetration > manB.penetration) ? std::optional<Manifold>(manA) : std::optional<Manifold>(manB);
}