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

MapLoader::MapLoader()
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
                    if (texture->loadFromFile(/*xy::FileSystem::getResourcePath() + */path))
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

                //find the tile set and its associated texture

                //fill out other layer data
            }
        }

        return true;
    }

    return false;
}

