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

#include "MapParser.hpp"
#include "CollisionObject.hpp"
#include "ShapeUtils.hpp"
#include "WayPoint.hpp"

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/util/String.hpp>

#include <tmxlite/Map.hpp>
#include <tmxlite/Layer.hpp>

#include <limits>

//#define DRAW_DEBUG 1

namespace
{
    const std::array<std::string, CollisionObject::Type::Count> ObjectLayers =
    {
        "collision", "killzone", "space", "waypoints", "jump", "fence", "vehicle", "roid"
    };
    
    const std::array<sf::Color, CollisionObject::Type::Count> colours =
    {
        sf::Color::Yellow,
        sf::Color::Red,
        sf::Color::White,
        sf::Color::Magenta,
        sf::Color::Green,
        sf::Color::Cyan,
        sf::Color::Black,
        sf::Color::Black
    };
}

MapParser::MapParser(xy::Scene& scene)
    : m_scene(scene),
    m_waypointCount(0)
{

}

//public
bool MapParser::load(const std::string& path)
{   
    auto createObj = [&](const tmx::Object& obj, CollisionObject::Type type)->xy::Entity
    {
        sf::FloatRect aabb;
        auto pos = obj.getPosition();
        auto points = obj.getPoints();

        if (obj.getShape() == tmx::Object::Shape::Polygon
            || obj.getShape() == tmx::Object::Shape::Polyline)
        {
            //find width and height of AABB
            auto xExtremes = std::minmax_element(points.begin(), points.end(),
                [](const tmx::Vector2f& lhs, const tmx::Vector2f& rhs) 
                {
                    return lhs.x < rhs.x;
                });

            auto yExtremes = std::minmax_element(points.begin(), points.end(),
                [](const tmx::Vector2f& lhs, const tmx::Vector2f& rhs)
                {
                    return lhs.y < rhs.y;
                });

            aabb.left = xExtremes.first->x;
            aabb.top = yExtremes.first->y;
            aabb.width = xExtremes.second->x - aabb.left;
            aabb.height = yExtremes.second->y - aabb.top;
        }
        else
        {
            //convert AABB to points
            aabb.width = obj.getAABB().width;
            aabb.height = obj.getAABB().height;

            points =
            {
                tmx::Vector2f(aabb.left, aabb.top),
                {aabb.left + aabb.width, aabb.top},
                {aabb.left + aabb.width, aabb.top + aabb.height},
                {aabb.left, aabb.top + aabb.height}
            };
        }

        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(pos.x, pos.y);
        entity.addComponent<xy::BroadphaseComponent>().setArea(aabb);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Static);
        auto& collisionObj = entity.addComponent<CollisionObject>();
        collisionObj.type = type;

        std::vector<sf::Vector2f> sfPoints;
        for (auto p : points)
        {
            sfPoints.emplace_back(p.x, p.y);
        }
        collisionObj.applyVertices(sfPoints);

#ifdef DRAW_DEBUG
        if (!points.empty())
        {
            //draw edges
            auto& verts = entity.addComponent<xy::Drawable>().getVertices();
            
            for (auto p : collisionObj.vertices)
            {
                verts.push_back(sf::Vertex(p, colours[type]));
            }
            verts.push_back(sf::Vertex(collisionObj.vertices.front(), colours[type]));
            verts.push_back(sf::Vertex(collisionObj.vertices.front(), sf::Color::Transparent));

            //draw the normals
            for (auto i = 0u; i < collisionObj.vertices.size(); ++i)
            {
                verts.emplace_back(collisionObj.vertices[i], sf::Color::Transparent);
                verts.emplace_back(collisionObj.vertices[i], sf::Color::Red);
                verts.emplace_back(collisionObj.vertices[i] + (collisionObj.normals[i] * 30.f), sf::Color::Red);
                verts.emplace_back(collisionObj.vertices[i] + (collisionObj.normals[i] * 30.f), sf::Color::Transparent);
            }

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Drawable>().setDepth(100);
            entity.getComponent<xy::Drawable>().setPrimitiveType(sf::LineStrip);
        }
#endif // DRAW_DEBUG
        
        return entity;
    };

    //used for sorting waypoints
    std::vector<WayPoint*> waypoints;

    m_waypointCount = 0;
    tmx::Map map;
    if (map.load(xy::FileSystem::getResourcePath() + path))
    {
        //TODO bitset of found layers and return false if all necessary layers not found

        const auto& layers = map.getLayers();
        for (const auto& layer : layers)
        {
            if (layer->getType() == tmx::Layer::Type::Object)
            {
                const auto& objLayer = layer->getLayerAs<tmx::ObjectGroup>();
                auto name = xy::Util::String::toLower(objLayer.getName());
                for (auto i = 0u; i < ObjectLayers.size(); ++i)
                {
                    for (const auto& obj : objLayer.getObjects())
                    {
                        if (name == ObjectLayers[i])
                        {
                            switch (i)
                            {
                            default: break;
                            case CollisionObject::Fence:
                                //stash this somewhere so we know we need to draw an electric fence on the client
                                break;
                            case CollisionObject::Waypoint:
                                if (obj.getShape() != tmx::Object::Shape::Rectangle)
                                {
                                    xy::Logger::log("Invalid waypoint type, must be rectangular", xy::Logger::Type::Error);
                                    return false;
                                }

                                try
                                {
                                    std::int32_t id = std::stoi(obj.getName());

                                    const auto properties = obj.getProperties();
                                    auto result = std::find_if(properties.begin(), properties.end(), 
                                        [](const tmx::Property & p) 
                                        {
                                            return p.getName() == "rotation";
                                        });
                                    

                                    auto bounds = obj.getAABB();
                                    auto position = obj.getPosition();
                                    std::vector<sf::Vector2f> verts =
                                    {
                                        sf::Vector2f(-bounds.width / 2.f, -bounds.height / 2.f),
                                        sf::Vector2f(bounds.width / 2.f, -bounds.height / 2.f),
                                        sf::Vector2f(bounds.width / 2.f, bounds.height / 2.f),
                                        sf::Vector2f(-bounds.width / 2.f, bounds.height / 2.f)
                                    };

                                    auto entity = m_scene.createEntity();
                                    entity.addComponent<xy::Transform>().setPosition(position.x + (bounds.width / 2.f), position.y + (bounds.height / 2.f));
                                    entity.addComponent<CollisionObject>().type = CollisionObject::Waypoint;
                                    entity.getComponent<CollisionObject>().applyVertices(verts);
                                    entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Static);
                                    entity.getComponent<xy::BroadphaseComponent>().setArea({ -bounds.width / 2.f, -bounds.height / 2.f, bounds.width, bounds.height });
                                    entity.addComponent<WayPoint>().id = id;
                                    entity.getComponent<WayPoint>().nextPoint = entity.getComponent<xy::Transform>().getPosition(); //this is correctly processed below
                                    if (result != properties.end())
                                    {
                                        if (result->getType() == tmx::Property::Type::Float)
                                        {
                                            entity.getComponent<WayPoint>().rotation = result->getFloatValue();
                                        }
                                        else
                                        {
                                            xy::Logger::log("Waypoint " + obj.getName() + " rotation property of incorrect type.", xy::Logger::Type::Warning);
                                        }
                                    }
                                    else
                                    {
                                        xy::Logger::log("Waypoint " + obj.getName() + " rotation property missing", xy::Logger::Type::Warning);
                                    }

                                    m_waypointCount++;
                                    waypoints.push_back(&entity.getComponent<WayPoint>());

                                    if (id == 0)
                                    {
                                        m_startPosition = entity.getComponent<xy::Transform>().getPosition();
                                    }
#ifdef DRAW_DEBUG
                                    verts.push_back(verts.front());
                                    Shape::setPolyLine(entity.addComponent<xy::Drawable>(), verts, sf::Color::Cyan);
                                    entity.getComponent<xy::Drawable>().setDepth(100);
#endif
                                }
                                catch (...)
                                {
                                    xy::Logger::log("waypoint has invalid name " + obj.getName(), xy::Logger::Type::Error);
                                    return false;
                                }
                                break;
                            case CollisionObject::Jump:
                            case CollisionObject::Collision:
                            case CollisionObject::KillZone:
                            case CollisionObject::Space:
                                createObj(obj, static_cast<CollisionObject::Type>(i));
                                break;

                            }
                        }
                    }
                }
            }
            else if (layer->getType() == tmx::Layer::Type::Tile)
            {
                //const auto& tileLayer = layer->getLayerAs<tmx::TileLayer>();
                //TODO render layers
            }
        }

        //validate the waypoint data
        if (m_startPosition == sf::Vector2f())
        {
            xy::Logger::log("No start position was set - waypoints should be labelled from 0", xy::Logger::Type::Error);
            return false;
        }

        if (m_waypointCount < 2)
        {
            xy::Logger::log("No waypoints were loaded for map!", xy::Logger::Type::Error);
            return false;
        }

        //make sure we have all the correct indices
        //then calculate the vector to the next waypoint from
        //each one - this is how we know how far around the
        //track a player is.
        std::sort(waypoints.begin(), waypoints.end(), [](const WayPoint* a, const WayPoint* b) {return a->id < b->id; });
        for (auto i = 0u; i < waypoints.size() - 1; ++i)
        {
            if (waypoints[i + 1]->id - waypoints[i]->id != 1)
            {
                xy::Logger::log("Waypoints missing correct number at " + std::to_string(i), xy::Logger::Type::Error);
                return false;
            }

            waypoints[i]->nextPoint = waypoints[i + 1]->nextPoint - waypoints[i]->nextPoint;
            waypoints[i]->distance = xy::Util::Vector::length(waypoints[i]->nextPoint);
            waypoints[i]->nextPoint /= waypoints[i]->distance;            
        }
        waypoints.back()->nextPoint = m_startPosition - waypoints.back()->nextPoint;
        waypoints.back()->distance = xy::Util::Vector::length(waypoints.back()->nextPoint);
        waypoints.back()->nextPoint /= waypoints.back()->distance;

        return true; //TODO only if bitset matches
    }

    return false;
}

//private