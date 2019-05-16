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

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/util/String.hpp>

#include <tmxlite/Map.hpp>
#include <tmxlite/Layer.hpp>

#include <limits>

#define DRAW_DEBUG 1

MapParser::MapParser(xy::Scene& scene)
    :m_scene(scene)
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
            aabb.width = xExtremes.second->x;
            aabb.height = yExtremes.second->y;
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
        auto& segments = entity.addComponent<CollisionObject>().segments;
        entity.getComponent<CollisionObject>().type = type;

        if (points.size() > 1)
        {
            for (auto i = 0u; i < points.size() - 1; ++i)
            {
                //these are world space
                sf::Vector2f a(points[i].x + pos.x, points[i].y + pos.y);
                sf::Vector2f b(points[i + 1].x + pos.x, points[i + 1].y + pos.y);

                segments.emplace_back(std::make_pair(a, b));
            }
        }
#ifdef DRAW_DEBUG
        if (!points.empty())
        {
            auto& verts = entity.addComponent<xy::Drawable>().getVertices();
            
            for (auto p : points)
            {
                verts.push_back(sf::Vertex({ p.x, p.y }, MapConst::colours[type]));
            }
            verts.push_back(sf::Vertex({ points[0].x, points[0].y }, MapConst::colours[type]));

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Drawable>().setDepth(100);
            entity.getComponent<xy::Drawable>().setPrimitiveType(sf::LineStrip);
        }
#endif // DRAW_DEBUG
        
        return entity;
    };

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
                for (auto i = 0; i < MapConst::ObjectLayers.size(); ++i)
                {
                    for (const auto& obj : objLayer.getObjects())
                    {
                        if (name == MapConst::ObjectLayers[i])
                        {
                            switch (i)
                            {
                            default: break;
                            case MapConst::Fence:
                                createObj(obj, CollisionObject::Fence);
                                break;
                            case MapConst::Jump:
                                createObj(obj, CollisionObject::Jump);
                                break;
                            case MapConst::Collision:
                                createObj(obj, CollisionObject::Collision);
                                break;
                            case MapConst::KillZone:
                                createObj(obj, CollisionObject::KillZone);
                                break;
                            case MapConst::Space:
                                //createObj(obj, CollisionObject::Space);
                                break;
                            case MapConst::WayPoints:

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

        return true; //TODO only if bitset matches
    }

    return false;
}

//private