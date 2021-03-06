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
#include "Sprite3D.hpp"
#include "Camera3D.hpp"
#include "CommandIDs.hpp"
#include "GameConsts.hpp"
#include "VertexFunctions.hpp"
#include "MatrixPool.hpp"
#include "AnimationCallbacks.hpp"

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <xyginext/util/String.hpp>
#include <xyginext/audio/AudioScape.hpp>

#include <tmxlite/Layer.hpp>
#include <tmxlite/TileLayer.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/OpenGL.hpp>

#include <limits>

//#define DRAW_DEBUG 1

namespace
{
    const std::array<std::string, CollisionObject::Type::Count> ObjectLayers =
    {
        "collision", "killzone", "space", "waypoints", "jump", "props", "vehicle", "roid"
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
    m_waypointCount(0),
    m_startRotation(0.f),
    m_trackLength(0.f)
{
    m_layers = { nullptr, nullptr, nullptr, nullptr };
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
    m_trackLength = 0.f;
    m_layers = { nullptr, nullptr, nullptr, nullptr };
    if (m_map.load(xy::FileSystem::getResourcePath() + path))
    {
        //TODO bitset of found layers and return false if all necessary layers not found

        const auto& layers = m_map.getLayers();
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
                            case CollisionObject::Prop:
                            {
                                if (obj.getShape() == tmx::Object::Shape::Polyline)
                                {
                                    const auto& points = obj.getPoints();
                                    if (points.size() == 2)
                                    {
                                        auto start = obj.getPosition() + points[0];
                                        auto end = obj.getPosition() + points[1];

                                        if (obj.getType() == "fence")
                                        {
                                            Lightning lightning;
                                            lightning.start = { start.x, start.y };
                                            lightning.end = { end.x, end.y };
                                            m_fences.push_back(lightning);
                                        }
                                        else if (obj.getType() == "chevron")
                                        {
                                            m_chevrons.emplace_back(std::make_pair(sf::Vector2f(start.x, start.y), sf::Vector2f(end.x, end.y)));
                                        }
                                        else if (obj.getType() == "barrier")
                                        {
                                            m_barriers.emplace_back(std::make_pair(sf::Vector2f(start.x, start.y), sf::Vector2f(end.x, end.y)));
                                        }
                                    }
                                }
                                else if (obj.getShape() == tmx::Object::Shape::Point)
                                {
                                    if (obj.getType() == "bollard")
                                    {
                                        m_bollards.emplace_back(obj.getPosition().x, obj.getPosition().y);
                                    }
                                    else
                                    {
                                        m_pylons.emplace_back(obj.getPosition().x, obj.getPosition().y);
                                    }
                                }
                            }
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
                const auto& tileLayer = layer->getLayerAs<tmx::TileLayer>();
                //save this layer if valid - that way we can only render if we're
                //on the client, plus check if all the necessary layers were found after loading
                auto name = xy::Util::String::toLower(tileLayer.getName());
                if (name == "track")
                {
                    m_layers[Track] = &tileLayer;
                }
                else if (name == "neon")
                {
                    m_layers[Neon] = &tileLayer;
                }
                else if (name == "detail")
                {
                    m_layers[Detail] = &tileLayer;
                }
                else if (name == "normal")
                {
                    m_layers[Normal] = &tileLayer;
                }
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
            waypoints[i]->nextDistance = xy::Util::Vector::length(waypoints[i]->nextPoint);
            waypoints[i]->nextPoint /= waypoints[i]->nextDistance;
            waypoints[i]->trackDistance = m_trackLength;

            m_trackLength += waypoints[i]->nextDistance;
        }
        waypoints.back()->nextPoint = m_startPosition - waypoints.back()->nextPoint;
        waypoints.back()->nextDistance = xy::Util::Vector::length(waypoints.back()->nextPoint);
        waypoints.back()->nextPoint /= waypoints.back()->nextDistance;
        waypoints.back()->trackDistance = m_trackLength;

        m_trackLength += waypoints.back()->nextDistance;
        m_startRotation = waypoints.front()->rotation;


        //make sure we found all the image layers
        if(std::any_of(m_layers.begin(), m_layers.end(), [](const tmx::TileLayer* l){return l == nullptr;}))
        {
            xy::Logger::log("Missing one or more image layers", xy::Logger::Type::Error);
            return false;
        }

        auto mapSize = m_map.getTileCount() * m_map.getTileSize();
        m_size = { static_cast<float>(mapSize.x), static_cast<float>(mapSize.y) };

        return true;
    }

    return false;
}

void MapParser::renderLayers(std::array<sf::RenderTexture, 2u>& targets) const
{
    //TODO instead of rendering HUGE textures which eat all the VRAM, use the
    //tilemap shader implemented in the platformer project? This will save considerably
    //on VRAM but will actually increase the complexity and number of draw calls...
    //for example the track layer will have to be rendered 3 times, and carry the
    //overhead of texture switching and extra shader binding

    //tileset data
    TilesetInfo tsi;
    tsi.tileSets = &m_map.getTilesets();

    for (const auto& ts : *tsi.tileSets)
    {
        std::unique_ptr<sf::Texture> tex = std::make_unique<sf::Texture>();
        if (!tex->loadFromFile(ts.getImagePath()))
        {
            xy::Logger::log("failed loading tile set image " + ts.getImagePath());
            return;
        }
        else
        {
            tsi.textures.push_back(std::move(tex));
        }
    }


    auto mapSize = m_map.getTileCount() * m_map.getTileSize();

    targets[0].create(mapSize.x, mapSize.y);
    targets[0].clear(sf::Color::Transparent);
    renderLayer(targets[0], m_layers[Track], tsi);
    renderLayer(targets[0], m_layers[Detail], tsi);
    renderLayer(targets[0], m_layers[Neon], tsi);
    targets[0].display();

    //targets[1].create(mapSize.x, mapSize.y);
    //targets[1].clear(sf::Color::Transparent);
    //renderLayer(targets[1], m_layers[Neon], tsi);
    //targets[1].display();

    targets[1].create(mapSize.x, mapSize.y);
    targets[1].clear(sf::Color::Transparent);
    renderLayer(targets[1], m_layers[Normal], tsi);
    targets[1].display();
}

void MapParser::addProps(MatrixPool& matrixPool, xy::AudioResource& ar, xy::ShaderResource& shaders, xy::ResourceHandler& resources, const std::array<std::size_t, TextureID::Game::Count>& textureIDs)
{
    //auto cameraEntity = m_scene.getActiveCamera();

    xy::AudioScape audioScape(ar);
    audioScape.loadFromFile("assets/sound/map.xas");

    //NOTE viewProj matrices are now set in the render path class

    //electric fences
    const auto& fences = m_fences;
    const auto& fenceTexture = resources.get<sf::Texture>(textureIDs[TextureID::Game::Fence]);
    auto texSize = sf::Vector2f(fenceTexture.getSize());

    for (const auto& f : fences)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>(); //points are in world space.
        entity.addComponent<xy::Drawable>().setFilterFlags(GameConst::FilterFlags::All);
        entity.addComponent<Lightning>() = f;

        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(f.start);
        entity.addComponent<xy::Drawable>().setDepth(1000);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().setTexture(&fenceTexture);
        entity.getComponent<xy::Drawable>().setShader(&shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(matrixPool).depth = GameConst::FenceHeight;
        //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createBillboard(f.start, f.end, entity.getComponent<Sprite3D>().depth, texSize);
        entity.getComponent<xy::Drawable>().updateLocalBounds();

        entity.addComponent<xy::AudioEmitter>() = audioScape.getEmitter("fence");
        entity.getComponent<xy::AudioEmitter>().play();
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Game::Audio;
    }

    //chevrons
    const auto& chevrons = m_chevrons;
    const auto& chevronTexture = resources.get<sf::Texture>(textureIDs[TextureID::Game::Chevron]);
    texSize = sf::Vector2f(chevronTexture.getSize());

    for (const auto& [start, end] : chevrons)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(start);
        entity.addComponent<xy::Drawable>().setDepth(1000);
        entity.getComponent<xy::Drawable>().setTexture(&chevronTexture);
        entity.getComponent<xy::Drawable>().setShader(&shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(matrixPool).depth = GameConst::ChevronHeight;
        //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createBillboard(start, end, entity.getComponent<Sprite3D>().depth, texSize);
        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }

    //race barriers
    const auto& barriers = m_barriers;
    auto& barrierTexture = resources.get<sf::Texture>(textureIDs[TextureID::Game::Barrier]);
    barrierTexture.setRepeated(true);
    texSize = sf::Vector2f(barrierTexture.getSize());

    for (const auto& [start, end] : barriers)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(start);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
        entity.getComponent<xy::Drawable>().setTexture(&barrierTexture);
        entity.getComponent<xy::Drawable>().setShader(&shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(matrixPool).depth = GameConst::BarrierHeight;
        //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createBillboard(start, end, entity.getComponent<Sprite3D>().depth, { xy::Util::Vector::length(end - start), texSize.y });
        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }

    //electric pylons
    const auto& pylons = m_pylons;
    auto& pylonTexture = resources.get<sf::Texture>(textureIDs[TextureID::Game::Pylon]);
    texSize = sf::Vector2f(pylonTexture.getSize());
    for (auto p : pylons)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(p);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().setTexture(&pylonTexture);
        entity.getComponent<xy::Drawable>().setShader(&shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(matrixPool).depth = GameConst::PylonHeight;
        //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createPylon(texSize);

        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }


    //bollards.
    const auto& bollards = m_bollards;
    auto& bollardTexture = resources.get<sf::Texture>(textureIDs[TextureID::Game::Bollard]);
    texSize = sf::Vector2f(bollardTexture.getSize());
    for (auto b : bollards)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(b);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().setTexture(&bollardTexture);
        entity.getComponent<xy::Drawable>().setShader(&shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(matrixPool).depth = GameConst::BollardHeight;
        //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createCylinder(GameConst::BollardRadius, texSize, GameConst::BollardHeight);

        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }

    //rings around start area
    auto temp = resources.load<sf::Texture>("assets/images/start_field.png");
    resources.get<sf::Texture>(temp).setSmooth(true);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(getStartPosition().first);
    entity.addComponent<xy::Drawable>().setDepth(1000);
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.getComponent<xy::Drawable>().setTexture(&resources.get<sf::Texture>(temp));
    entity.getComponent<xy::Drawable>().setShader(&shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<Sprite3D>(matrixPool).depth = 50.f;
    //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

    texSize = sf::Vector2f(entity.getComponent<xy::Drawable>().getTexture()->getSize());
    entity.getComponent<xy::Drawable>().getVertices() = createStartField(texSize.x, entity.getComponent<Sprite3D>().depth);

    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function = StartRingAnimator();

    //lap line
    auto& lapTexture = resources.get<sf::Texture>(textureIDs[TextureID::Game::LapLine]);
    texSize = sf::Vector2f(lapTexture.getSize());

    sf::Vector2f offset = xy::Util::Vector::rotate({ GameConst::LapArchOffset, 0.f }, getStartPosition().second);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(getStartPosition().first + offset);
    entity.getComponent<xy::Transform>().setRotation(getStartPosition().second);
    entity.addComponent<xy::Drawable>().setDepth(1000);
    entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
    entity.getComponent<xy::Drawable>().setTexture(&lapTexture);
    entity.getComponent<xy::Drawable>().setShader(&shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<Sprite3D>(matrixPool).depth = texSize.y / 2.f;
    //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

    entity.getComponent<xy::Drawable>().getVertices() = createLapLine();

    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Game::LapLine;
}

//private
void MapParser::renderLayer(sf::RenderTarget& target, const tmx::TileLayer* layer, const TilesetInfo& tsi) const
{
    XY_ASSERT(layer != nullptr, "Map not correctly loaded!");
    
    sf::IntRect textureRect(0, 0, (m_map.getTileSize().x), (m_map.getTileSize().y));
    sf::Sprite tileSprite;

    //render layer
    const auto& tileSets = *tsi.tileSets;
    const auto& tiles = layer->getTiles();

    for (auto y = 0u; y < m_map.getTileCount().y; ++y)
    {
        for (auto x = 0u; x < m_map.getTileCount().x; ++x)
        {
            auto posX = static_cast<float>(x * m_map.getTileSize().x);
            auto posY = static_cast<float>(y * m_map.getTileSize().y);
            sf::Vector2f position(posX, posY);

            tileSprite.setPosition(position);

            auto tileID = tiles[y * m_map.getTileCount().x + x].ID;

            if (tileID == 0)
            {
                continue; //empty tile
            }

            std::size_t i = 0;
            for (; i < tsi.tileSets->size(); ++i)
            {
                if (tileID >= tileSets[i].getFirstGID() && tileID <= tileSets[i].getLastGID())
                {
                    break;
                }
            }

            auto relativeID = tileID - tileSets[i].getFirstGID();
            auto tileX = relativeID % tileSets[i].getColumnCount();
            auto tileY = relativeID / tileSets[i].getColumnCount();
            textureRect.left = tileX * tileSets[i].getTileSize().x;
            textureRect.top = tileY * tileSets[i].getTileSize().y;

            tileSprite.setTexture(*tsi.textures[i]);
            tileSprite.setTextureRect(textureRect);

            target.draw(tileSprite);
        }
    }
}