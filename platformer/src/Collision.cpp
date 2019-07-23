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


#include "Collision.hpp"

#include <xyginext/ecs/components/Transform.hpp>

std::optional<Manifold> intersectsAABB(sf::FloatRect a, sf::FloatRect b)
{
    sf::FloatRect overlap;
    if (a.intersects(b, overlap))
    {
        sf::Vector2f centreA(a.left + (a.width / 2.f), a.top + (a.height / 2.f));
        sf::Vector2f centreB(b.left + (b.width / 2.f), b.top + (b.height / 2.f));
        sf::Vector2f collisionNormal = centreB - centreA;

        Manifold manifold;

        if (overlap.width < overlap.height)
        {
            manifold.normal.x = (collisionNormal.x < 0) ? 1.f : -1.f;
            manifold.penetration = overlap.width;
        }
        else
        {
            manifold.normal.y = (collisionNormal.y < 0) ? 1.f : -1.f;
            manifold.penetration = overlap.height;
        }

        return manifold;
    }
    return std::nullopt;
}

std::optional<Manifold> intersectsSAT(xy::Entity, xy::Entity)
{
    return std::nullopt;
}

sf::FloatRect boundsToWorldSpace(sf::FloatRect bounds, const xy::Transform& tx)
{
    auto scale = tx.getScale();
    bounds.left *= scale.x;
    bounds.width *= scale.x;
    bounds.top *= scale.y;
    bounds.height *= scale.y;
    bounds.left += tx.getPosition().x;
    bounds.top += tx.getPosition().y;

    return bounds;
}