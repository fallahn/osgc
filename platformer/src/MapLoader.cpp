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

#include <xyginext/core/FileSystem.hpp>
#include <xyginext/core/Log.hpp>

#include <SFML/Graphics/Image.hpp>

#include <tmxlite/Map.hpp>
#include <tmxlite/Layer.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>

#include <map>

MapLoader::MapLoader()
    : m_tileSize    (0.f)
{

}

//public
bool MapLoader::load(const std::string& file)
{
    m_indexMaps.clear();
    m_mapLayers.clear();
    m_tilesets.clear();

    tmx::Map map;

    if (map.load(xy::FileSystem::getResourcePath() + "assets/maps/" + file))
    {
        m_tileSize = static_cast<float>(map.getTileSize().x);

        //load the tile sets and associated textures
        std::map<const tmx::Tileset*, sf::Texture*> tsTextures;
        const auto& tileSets = map.getTilesets();
        for (const auto& ts : tileSets)
        {
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

        //this makes some basic assumptions such as a layer only uses a single
        //tile set, and that all tiles within the set are the same size.
        const auto& layers = map.getLayers();
        for (const auto& layer : layers)
        {
            if (layer->getType() == tmx::Layer::Type::Tile)
            {
                const auto& tileLayer = layer->getLayerAs<tmx::TileLayer>();
                auto& mapLayer = m_mapLayers.emplace_back();

                //create the index map for the layer
                std::uint32_t firstID = std::numeric_limits<std::uint32_t>::max();

                const auto& tiles = tileLayer.getTiles();
                std::vector<sf::Uint8> tileIDs(tiles.size());

                for (auto i = 0u; i < tiles.size(); ++i)
                {
                    //REMEMBER tmx tile IDs start at 1 (0 is an empty tile)
                    //we'll account for this in the tile shader.
                    tileIDs[i] = static_cast<sf::Uint8>(tiles[i].ID);

                    if (tiles[i].ID < firstID)
                    {
                        firstID = std::max(1u, tiles[i].ID);
                    }
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


                //correct the tile ID is necessary and load into a texture via an image
                auto layerSize = map.getTileCount();

                sf::Image indexImage;
                indexImage.create(layerSize.x, layerSize.y);

                for (auto i = 0u; i < tileIDs.size(); ++i)
                {
                    //if the layer tile set ID doesn't start at 1 we need to subtract the
                    //min ID from the tile ID before uploading it to the index texture
                    tileIDs[i] -= (tileset->getFirstGID() - 1);

                    auto x = i % layerSize.x;
                    auto y = i / layerSize.x;

                    indexImage.setPixel(x, y, { tileIDs[i], 255,255 });
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
            }

            //get the shapes which make up the map
            else if (layer->getType() == tmx::Layer::Type::Object)
            {
                auto toFloatRect = [](tmx::FloatRect rect)->sf::FloatRect
                {
                    return { rect.left, rect.top, rect.width, rect.height };
                };

                const auto& objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
                for (const auto& object : objects)
                {
                    auto shape = object.getShape();
                    switch (shape)
                    {
                    default: break;
                    case tmx::Object::Shape::Rectangle:
                    {
                        auto& collision = m_collisionShapes.emplace_back();
                        collision.type = CollisionShape::Solid;
                        collision.shape = CollisionShape::Rectangle;
                        collision.aabb = toFloatRect(object.getAABB());
                        collision.collisionFlags = CollisionShape::Player | CollisionShape::Foot | CollisionShape::LeftHand | CollisionShape::RightHand;
                    }
                        break;
                    }
                }
            }
        }

        return true;
    }

    return false;
}

