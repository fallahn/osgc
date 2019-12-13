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

#include "MapLoader.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/core/FileSystem.hpp>
#include <xyginext/core/Log.hpp>

#include <xyginext/util/String.hpp>

#include <SFML/Graphics/Image.hpp>

#include <tmxlite/Map.hpp>
#include <tmxlite/Layer.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>

#include <map>

MapLoader::MapLoader()
    : m_tileSize    (0.f),
    m_nextTheme     ("gearboy")
{

}

//public
bool MapLoader::load(const std::string& file)
{
    m_indexMaps.clear();
    m_mapLayers.clear();
    m_tilesets.clear();
    m_tileSize = 64.f;
    m_spawnPoint = {};

    tmx::Map map;

    if (map.load(xy::FileSystem::getResourcePath() + "assets/maps/" + file))
    {
        m_tileSize = static_cast<float>(map.getTileSize().x);
        m_mapSize = { float(map.getTileCount().x * map.getTileSize().x), float(map.getTileCount().y * map.getTileSize().y) };

        //load the tile sets and associated textures
        std::map<const tmx::Tileset*, sf::Texture*> tsTextures;
        const auto& tileSets = map.getTilesets();
        for (const auto& ts : tileSets)
        {
            if (ts.getTileCount() > 256)
            {
                //TODO could use green channel to store next 256 IDs
                xy::Logger::log("Tileset found with more than 256 tiles", xy::Logger::Type::Error);
                return false;
            }

            auto path = ts.getImagePath();
            if (!path.empty())
            {
                if (m_tilesets.count(path) != 0)
                {
                    tsTextures.insert(std::make_pair(&ts, m_tilesets[path].get()));
                }
                else
                {
                    std::unique_ptr<sf::Texture> texture = std::make_unique<sf::Texture>();
                    if (texture->loadFromFile(path))
                    {
                        tsTextures.insert(std::make_pair(&ts, texture.get()));
                        m_tilesets.insert(std::make_pair(path, std::move(texture)));
                    }
                    else
                    {
                        xy::Logger::log("Failed to load texture " + path, xy::Logger::Type::Error);
                        return false;
                    }
                }
            }
        }

        //need to make sure this is filled and has correct IDs
        std::vector<std::pair<sf::Vector2f, std::int32_t>> checkpoints;

        //this makes some basic assumptions such as a layer only uses a single
        //tile set, and that all tiles within the set are the same size.
        std::size_t exitCount = 0; //MUST have exactly 1
        const auto& layers = map.getLayers();
        for (const auto& layer : layers)
        {
            if (layer->getType() == tmx::Layer::Type::Tile)
            {
                const auto& tileLayer = layer->getLayerAs<tmx::TileLayer>();
                auto& mapLayer = m_mapLayers.emplace_back();

                //used in edge data calc
                static std::array<sf::Vector2f, 4u> p1Offsets =
                {
                    sf::Vector2f(),
                    sf::Vector2f(m_tileSize, 0.f),
                    sf::Vector2f(m_tileSize, m_tileSize),
                    sf::Vector2f(0.f, m_tileSize)
                };

                static std::array<sf::Vector2f, 4u> p2Offsets =
                {
                    sf::Vector2f(m_tileSize, 0.f),
                    sf::Vector2f(0.f, m_tileSize),
                    sf::Vector2f(-m_tileSize, 0.f),
                    sf::Vector2f(0.f, -m_tileSize),
                };

                //create the index map for the layer
                std::uint32_t firstID = std::numeric_limits<std::uint32_t>::max();

                const auto& tiles = tileLayer.getTiles();
                std::vector<sf::Uint32> tileIDs(tiles.size());

                //look to see if we have an extrusion property
                const auto& properties = tileLayer.getProperties();
                auto found = std::find_if(properties.begin(), properties.end(),
                    [](const tmx::Property& prop)
                    {
                        return prop.getName() == "extrude";
                    });

                bool extrude = (found != properties.end() && found->getBoolValue());

                for (auto i = 0u; i < tiles.size(); ++i)
                {
                    //REMEMBER tmx tile IDs start at 1 (0 is an empty tile)
                    //we'll account for this in the tile shader.
                    tileIDs[i] = /*static_cast<sf::Uint8>*/(tiles[i].ID);

                    if (tiles[i].ID < firstID
                        && tiles[i].ID > 0)
                    {
                        firstID = std::max(1u, tiles[i].ID);
                    }

                    //check surrounding tiles and add edges to the list if neighbour is empty
                    if ( extrude && tiles[i].ID > 0)
                    {
                        std::int32_t idx = static_cast<std::int32_t>(i);
                        std::int32_t xCount = static_cast<std::int32_t>(map.getTileCount().x);
                        std::array<std::int32_t, 4> neighbours =
                        {
                            idx - xCount,
                            (idx % xCount == xCount - 1) ? -1 : idx + 1,
                            idx + xCount,
                            (idx % xCount == 0) ? -1 : idx - 1,
                        };

                        for (auto j = 0; j < 4; ++j)
                        {
                            if (neighbours[j] > -1 && neighbours[j] < tiles.size())
                            {
                                if (tiles[neighbours[j]].ID == 0)
                                {
                                    auto& edge = mapLayer.edges.emplace_back();
                                    edge.facing = j;

                                    //calc first pos
                                    edge.vertices[0].position.x = static_cast<float>((i % xCount) * map.getTileSize().x);
                                    edge.vertices[0].position.y = static_cast<float>((i / xCount) * map.getTileSize().y);
                                    edge.vertices[0].position += p1Offsets[j];
                                    edge.vertices[1].position = edge.vertices[0].position + p2Offsets[j];
                                    edge.vertices[0].color.a = 0;
                                    edge.vertices[1].color.a = 0;

                                    //store the tile index so we can calc
                                    //UV once we know the tile set texture (below)
                                    edge.tileID = tiles[i].ID;
                                }
                            }
                        }
                    }
                }

                if (firstID == std::numeric_limits<std::uint32_t>::max())
                {
                    xy::Logger::log("Do you have an empty tile layer?", xy::Logger::Type::Warning);
                }

                //find the tile set and its associated texture
                auto result = std::find_if(tsTextures.begin(), tsTextures.end(),
                    [firstID](const std::pair<const tmx::Tileset*, sf::Texture*>& p)
                    {
                        return (firstID >= p.first->getFirstGID() && firstID <= p.first->getLastGID());
                    });

                if (result == tsTextures.end())
                {
                    xy::Logger::log("Missing tile set for layer..?", xy::Logger::Type::Error);
                    return false;
                }

                auto [tileset, texture] = *result;

                //find the UV coord for the tile edges
                auto xCount = texture->getSize().x / tileset->getTileSize().x;
                for (auto& edge : mapLayer.edges)
                {
                    auto idx = (edge.tileID - tileset->getFirstGID());// -1;

                    edge.vertices[0].texCoords.x = static_cast<float>((idx % xCount) * map.getTileSize().x);
                    edge.vertices[0].texCoords.y = static_cast<float>((idx / xCount) * map.getTileSize().y);
                    edge.vertices[0].texCoords += p1Offsets[edge.facing];
                    edge.vertices[1].texCoords = edge.vertices[0].texCoords + p2Offsets[edge.facing];                  
                }

                //correct the tile ID is necessary and load into a texture via an image
                auto layerSize = map.getTileCount();

                sf::Image indexImage;
                indexImage.create(layerSize.x, layerSize.y);

                for (auto i = 0u; i < tileIDs.size(); ++i)
                {
                    //if the layer tile set ID doesn't start at 1 we need to subtract the
                    //min ID from the tile ID before uploading it to the index texture
                    if (tileIDs[i] >= tileset->getFirstGID())
                    {
                        tileIDs[i] -= (tileset->getFirstGID() - 1);
                    }
                    auto x = i % layerSize.x;
                    auto y = i / layerSize.x;

                    indexImage.setPixel(x, y, { static_cast<sf::Uint8>(tileIDs[i]), 255,255 });
                }

                m_indexMaps.emplace_back() = std::make_unique<sf::Texture>();
                m_indexMaps.back()->loadFromImage(indexImage);
                mapLayer.indexMap = m_indexMaps.back().get();
                mapLayer.tileSet = texture;

                //fill out other layer data
                mapLayer.layerSize = sf::Vector2f(static_cast<float>(layerSize.x * tileset->getTileSize().x),
                    static_cast<float>(layerSize.y * tileset->getTileSize().y));
                mapLayer.tileSetSize = { static_cast<float>(texture->getSize().x / tileset->getTileSize().x),
                    static_cast<float>(texture->getSize().y / tileset->getTileSize().y) };

                //std::cout << mapLayer.edges.size() << " edges were found\n";
                //std::cout << "of a possible " << tiles.size() * 4 << "\n";
            }

            //get the shapes which make up the map
            else if (layer->getType() == tmx::Layer::Type::Object)
            {
                auto toFloatRect = [](tmx::FloatRect rect)->sf::FloatRect
                {
                    return { rect.left, rect.top, rect.width, rect.height };
                };

                auto getID = [](const tmx::Object obj)->std::optional<std::int32_t>
                {
                    const auto& properties = obj.getProperties();
                    auto result = std::find_if(properties.begin(), properties.end(),
                        [](const tmx::Property& prop)
                        {
                            return prop.getName() == "id";
                        });

                    if (result == properties.end())
                    {
                        xy::Logger::log("object " + obj.getType() + " found with no ID", xy::Logger::Type::Warning);
                        return std::nullopt;
                    }

                    return { result->getIntValue() };
                };

                const auto& objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
                for (const auto& object : objects)
                {
                    auto shape = object.getShape();
                    switch (shape)
                    {
                    default: break;
                    case tmx::Object::Shape::Polyline:
                    {
                        auto type = xy::Util::String::toLower(object.getType());
                        if (type == "enemy")
                        {
                            auto id = getID(object);
                            if (id)
                            {
                                const auto& points = object.getPoints();
                                if (points.size() > 1)
                                {
                                    auto pos = object.getPosition();
                                    sf::Vector2f start(pos.x + points[0].x, pos.y + points[0].y);
                                    sf::Vector2f end(pos.x + points.back().x, pos.y + points.back().y);

                                    m_enemySpawns.emplace_back(
                                        std::make_pair(
                                            std::make_pair(start, end), 
                                            *id)
                                    );
                                }
                            }
                        }
                        else if (type == "platpath")
                        {
                            auto id = getID(object);
                            if (id)
                            {
                                const auto& points = object.getPoints();
                                if (points.size() > 1)
                                {
                                    auto pos = object.getPosition();
                                    PlatformPath path;
                                    for (auto p : points)
                                    {
                                        path.emplace_back(pos.x + p.x, pos.y + p.y);
                                    }
                                    m_platformPaths.insert(std::make_pair(*id, path));
                                }
                            }
                        }
                    }
                        break;
                    case tmx::Object::Shape::Rectangle:
                    {
                        auto& collision = m_collisionShapes.emplace_back();
                        collision.shape = CollisionShape::Rectangle;
                        collision.aabb = toFloatRect(object.getAABB());

                        auto type = xy::Util::String::toLower(object.getType());
                        if (type == "solid")
                        {
                            collision.collisionFlags = CollisionShape::Player | CollisionShape::Foot | CollisionShape::LeftHand | CollisionShape::RightHand;
                            collision.type = CollisionShape::Solid;
                        }
                        else if (type == "fluid")
                        {
                            auto id = getID(object);
                            if (id)
                            {
                                collision.type = CollisionShape::Fluid;
                                collision.collisionFlags = CollisionShape::Player;
                                collision.ID = *id;
                            }
                        }
                        else if (type == "spikes")
                        {
                            collision.type = CollisionShape::Spikes;
                            collision.collisionFlags = CollisionShape::Player;
                        }
                        else if (type == "checkpoint")
                        {
                            auto id = getID(object);
                            if (id)
                            {
                                collision.type = CollisionShape::Checkpoint;
                                collision.collisionFlags = CollisionShape::Player;
                                collision.ID = *id;

                                checkpoints.emplace_back(std::make_pair(sf::Vector2f(object.getPosition().x, object.getPosition().y), *id));
                            }
                            else
                            {
                                m_collisionShapes.pop_back();
                            }
                        }
                        else if (type == "exit")
                        {
                            const auto& properties = object.getProperties();
                            auto result = std::find_if(properties.begin(), properties.end(),
                                [](const tmx::Property& p)
                                {
                                    return p.getName() == "next_map";
                                });

                            if (result == properties.end())
                            {
                                xy::Logger::log("No next map property found", xy::Logger::Type::Error);
                                return false;
                            }
                            m_nextMap = result->getStringValue();

                            result = std::find_if(properties.begin(), properties.end(),
                                [](const tmx::Property& p)
                                {
                                    return p.getName() == "next_theme";
                                });

                            if (result != properties.end())
                            {
                                m_nextTheme = result->getStringValue();
                                if (!xy::FileSystem::directoryExists(xy::FileSystem::getResourcePath() + "assets/images/" + m_nextTheme))
                                {
                                    xy::Logger::log(m_nextTheme + ": invalid theme property", xy::Logger::Type::Error);
                                    return false;
                                }
                            }

                            auto id = getID(object);
                            if (id)
                            {
                                collision.ID = *id;
                            }
                            else
                            {
                                collision.ID = 0;
                            }

                            collision.type = CollisionShape::Exit;
                            collision.collisionFlags = CollisionShape::Player;

                            exitCount++;
                        }
                        else if (type == "collectible")
                        {
                            auto id = getID(object);
                            if (id)
                            {
                                collision.type = CollisionShape::Collectible;
                                collision.collisionFlags = CollisionShape::Player;
                                collision.ID = *id;
                            }
                            else
                            {
                                m_collisionShapes.pop_back();
                            }
                        }
                        else if (type == "dialogue")
                        {
                            const auto& properties = object.getProperties();
                            auto result = std::find_if(properties.begin(), properties.end(),
                                [](const tmx::Property& prop)
                                {
                                    return prop.getName() == "file";
                                });

                            if (result != properties.end())
                            {
                                auto str = result->getStringValue();
                                if (!str.empty())
                                {
                                    collision.type = CollisionShape::Dialogue;
                                    collision.collisionFlags = CollisionShape::Player;
                                    collision.ID = static_cast<std::int32_t>(m_dialogueFiles.size());

                                    m_dialogueFiles.push_back(str);
                                }
                                else
                                {
                                    m_collisionShapes.pop_back();
                                }
                            }
                            else
                            {
                                m_collisionShapes.pop_back();
                            }
                        }
                        else if (type == "mplat")
                        {
                            auto id = getID(object);
                            if (id)
                            {
                                collision.type = CollisionShape::MPlat;
                                collision.collisionFlags = CollisionShape::Player | CollisionShape::Foot | CollisionShape::LeftHand | CollisionShape::RightHand;
                                collision.ID = *id;
                            }
                            else
                            {
                                m_collisionShapes.pop_back();
                            }
                        }

                        //props
                        else if (type == "prop")
                        {
                            m_collisionShapes.pop_back();
                            
                            auto propID = PropID::Torch;
                            auto id = getID(object);
                            if (id)
                            {
                                propID = static_cast<PropID::Prop>(*id);
                            }

                            auto pos = object.getPosition();
                            auto aabb = object.getAABB();
                            m_props.emplace_back(std::make_pair(propID, sf::FloatRect(pos.x, pos.y, aabb.width, aabb.height)));
                        }

                        else
                        {
                            //not valid, remove
                            m_collisionShapes.pop_back();
                        }
                    }
                        break;
                    }
                }
            }
        }

        if (exitCount != 1)
        {
            xy::Logger::log(std::to_string(exitCount) + ": invalid exit count");
            return false;
        }

        //check valid check points and at least one spawn
        if (checkpoints.empty())
        {
            xy::Logger::log("No Checkpoints found", xy::Logger::Type::Error);
            return false;
        }
        std::sort(checkpoints.begin(), checkpoints.end(),
            [](const std::pair<sf::Vector2f, std::int32_t>& a, const std::pair<sf::Vector2f, std::int32_t>& b)
            {return a.second < b.second; });
        m_spawnPoint = checkpoints[0].first;

        return true;
    }

    return false;
}