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

#include "TrailSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

TrailSystem::TrailSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(TrailSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<Trail>();
}

//public
void TrailSystem::process(float dt)
{
    auto entities = getEntities();
    for (auto entity : entities)
    {
        auto& trail = entity.getComponent<Trail>();
        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();

        for (auto i = 0u; i < trail.points.size(); ++i)
        {
            trail.points[i].lifetime = std::max(0.f, trail.points[i].lifetime - dt);
            if (!trail.points[i].sleeping)
            {
                if (trail.points[i].lifetime == 0)
                {
                    if (trail.parent.isValid())
                    {
                        trail.points[i].position = trail.parent.getComponent<xy::Transform>().getPosition();
                        trail.points[i].lifetime = Trail::Point::MaxLifetime;
                    }
                    else
                    {
                        trail.points[i].sleeping = true;
                        trail.sleepCount++;
                    }
                    trail.oldestPoint = (trail.oldestPoint + 1) % trail.points.size();
                } 
            }
        }
        //rebuild vert array here making sure to start with the oldest
        verts.clear();

        auto i = trail.oldestPoint;
        verts.emplace_back(trail.points[i].position, trail.colour, sf::Vector2f(12.5f, 0.f));
        verts.back().color.a = static_cast<sf::Uint8>(255.f * (trail.points[i].lifetime / Trail::Point::MaxLifetime));
        i = (i + 1) % trail.points.size();

        for (auto j = 0u; j < trail.points.size(); ++j)
        {
            std::size_t next = (i + 1) % trail.points.size();
            
            if (next == trail.oldestPoint)
            {
                //we're at the end so go the other way
                next = (i + (trail.points.size() - 1)) % trail.points.size();
            }
            
            sf::Vector2f direction = trail.points[next].position - trail.points[i].position;
            sf::Vector2f vertPos(direction.y, -direction.x);
            vertPos *= 0.1f; //this is a kludge to save normalising the perp and resizing to a specific size.

            verts.emplace_back(vertPos + trail.points[i].position, trail.colour);
            verts.back().color.a = static_cast<sf::Uint8>(255.f * (trail.points[i].lifetime / Trail::Point::MaxLifetime));

            verts.emplace_back(-vertPos + trail.points[i].position, trail.colour, sf::Vector2f(13.f, 0.f));
            verts.back().color.a = static_cast<sf::Uint8>(255.f * (trail.points[i].lifetime / Trail::Point::MaxLifetime));

            i = next;
        }

        drawable.updateLocalBounds();

        if (trail.sleepCount == trail.points.size())
        {
            getScene()->destroyEntity(entity);
        }
    }
}

//private
void TrailSystem::onEntityAdded(xy::Entity entity)
{
    auto& trail = entity.getComponent<Trail>();

    XY_ASSERT(trail.parent.isValid(), "Can't init a trail without a parent");

    auto& drawable = entity.getComponent<xy::Drawable>();
    //drawable.getVertices().resize(trail.points.size());
    drawable.setPrimitiveType(sf::TriangleStrip);

    float lifeStep = Trail::Point::MaxLifetime / trail.points.size();
    auto position = trail.parent.getComponent<xy::Transform>().getPosition();

    for (auto i = 0u; i < trail.points.size(); ++i)
    {
        trail.points[i].lifetime = i * lifeStep;
        trail.points[i].position = position;
    }
}