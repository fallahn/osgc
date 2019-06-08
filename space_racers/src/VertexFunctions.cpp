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

#include "VertexFunctions.hpp"

#include <xyginext/core/Assert.hpp>
#include <xyginext/util/Vector.hpp>

#include <algorithm>

std::vector<sf::Vertex> convertData(std::vector<sf::Vector3f>& positions, const std::vector<sf::Vector3f>& normals, const std::vector<sf::Vector2f>& UVs)
{
    XY_ASSERT(!positions.empty(), "positions are empty!");
    XY_ASSERT(normals.size() == positions.size() || normals.empty(), "incorrect normal vector size");
    XY_ASSERT(UVs.size() == positions.size() || UVs.empty(), "incorrect UVs size");

    //find the max height of the model, and correct if it goes below Z 0
    auto [smallest, largest] = std::minmax_element(positions.begin(), positions.end(), 
        [](sf::Vector3f l, sf::Vector3f r)
        {
            return l.z < r.z;
        });

    if (smallest->z < 0)
    {
        auto correction = -smallest->z;
        for (auto& v : positions)
        {
            v.z += correction;
        }
    }

    std::vector<sf::Vertex> retVal;

    //convert the data
    for (auto i = 0u; i < positions.size(); ++i)
    {
        auto& vert = retVal.emplace_back();
        vert.position = { positions[i].x, positions[i].y };

        float depth = positions[i].z / largest->z;
        depth *= 255.f;

        vert.color.a = static_cast<sf::Uint8>(depth);

        if (!normals.empty())
        {
            //this assumes the normal vectors are, uh, normalised.
            //not sure how to assert for this...
            auto normal = normals[i];
            normal.x += 1.f;
            normal.x /= 2.f;

            normal.y += 1.f;
            normal.y /= 2.f;

            normal.z += 1.f;
            normal.z /= 2.f;

            vert.color.r = static_cast<sf::Uint8>(normal.x * 255.f);
            vert.color.g = static_cast<sf::Uint8>(normal.y * 255.f);
            vert.color.b = static_cast<sf::Uint8>(normal.z * 255.f);
        }

        if (!UVs.empty())
        {
            //on the other hand this assums the UVs are NOT normalised
            //as usual as this would require texture size data to convert
            //to SFML coordinate (which has probably already been done anyway)
            vert.texCoords = UVs[i];
        }
    }

    return retVal;
}

std::vector<sf::Vertex> createStartField(float radius, float height)
{
    std::vector<sf::Vector3f> positions;
    std::vector<sf::Vector2f> UVs;

    auto layerCount = 3;
    for (auto i = 0; i < layerCount; ++i)
    {
        positions.emplace_back(-radius, -radius, i * (height / layerCount));
        positions.emplace_back(radius, -radius, i * (height / layerCount));
        positions.emplace_back(radius, radius, i * (height / layerCount));
        positions.emplace_back(-radius, radius, i * (height / layerCount));

        UVs.emplace_back(0.f, 0.f);
        UVs.emplace_back(radius, 0.f);
        UVs.emplace_back(radius, radius);
        UVs.emplace_back(0.f, radius);
    }

    return convertData(positions, {}, UVs);
}

std::vector<sf::Vertex> createBillboard(sf::Vector2f start, sf::Vector2f end, float height, sf::Vector2f textureSize)
{
    //this offset is applied along the perpendicular of the top
    //vertices to angle the billboard back slightly just to make it
    //easier to view. It does depend on the start/end points being
    //drawn in the correct direction, however

    sf::Vector2f edge = end - start;

    sf::Vector2f offset = xy::Util::Vector::normalise(edge);
    offset = { offset.y, -offset.x };
    offset *= (height * 2.f);

    std::vector<sf::Vector3f> positions;
    std::vector<sf::Vector2f> UVs;

    positions.emplace_back(offset.x, offset.y, height);
    positions.emplace_back(offset.x + edge.x, offset.y + edge.y, height);
    positions.emplace_back(edge.x, edge.y, 0.f);
    positions.emplace_back(0.f, 0.f, 0.f);

    UVs.emplace_back(0.f, 0.f);
    UVs.emplace_back(textureSize.x, 0.f);
    UVs.emplace_back(textureSize);
    UVs.emplace_back(0.f, textureSize.y);

    return convertData(positions, {}, UVs);
}