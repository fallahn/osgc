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

#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>

#include <SFML/Graphics/Sprite.hpp>

#include <vector>
#include <memory>

namespace
{
    const sf::Vector2u TopMapSize(720, 960);
    const sf::Vector2u SideMapSize(0,0);
}

MapLoader::MapLoader()
{
    m_topTexture.create(TopMapSize.x, TopMapSize.y);
}

bool MapLoader::load(const std::string& path)
{
    tmx::Map map;
    if (map.load(xy::FileSystem::getResourcePath() + path))
    {
        //load tile sets
        std::vector<std::unique_ptr<sf::Texture>> textures;
        const auto& tileSets = map.getTilesets();

        for (const auto& ts : tileSets)
        {
            std::unique_ptr<sf::Texture> tex = std::make_unique<sf::Texture>();
            if (!tex->loadFromFile(xy::FileSystem::getResourcePath() +  + "assets/images/" + xy::FileSystem::getFileName(ts.getImagePath())))
            {
                xy::Logger::log("failed loading tile set image " + ts.getImagePath());
                return false;
            }
            else
            {
                textures.push_back(std::move(tex));
            }
        }

        //render layers
        sf::IntRect textureRect(0, 0, (map.getTileSize().x), (map.getTileSize().y));
        sf::Sprite tileSprite;

        m_topTexture.clear();

        const auto& layers = map.getLayers();
        for (const auto& layer : layers)
        {
            if (layer->getType() == tmx::Layer::Type::Tile)
            {
                const auto& tileLayer = layer->getLayerAs<tmx::TileLayer>();
                const auto& tiles = tileLayer.getTiles();

                for (auto y = 0u; y < map.getTileCount().y; ++y)
                {
                    for (auto x = 0u; x < map.getTileCount().x; ++x)
                    {
                        auto posX = static_cast<float>(x * map.getTileSize().x);
                        auto posY = static_cast<float>(y * map.getTileSize().y);
                        sf::Vector2f position(posX, posY);

                        tileSprite.setPosition(position);

                        auto tileID = tiles[y * map.getTileCount().x + x].ID;

                        if (tileID == 0)
                        {
                            continue; //empty tile
                        }

                        auto i = 0;
                        while (tileID < tileSets[i].getFirstGID())
                        {
                            ++i;
                        }

                        auto relativeID = tileID - tileSets[i].getFirstGID();
                        auto tileX = relativeID % tileSets[i].getColumnCount();
                        auto tileY = relativeID / tileSets[i].getColumnCount();
                        textureRect.left = tileX * tileSets[i].getTileSize().x;
                        textureRect.top = tileY * tileSets[i].getTileSize().y;

                        tileSprite.setTexture(*textures[0]);
                        tileSprite.setTextureRect(textureRect);

                        m_topTexture.draw(tileSprite);
                    }
                }
            }
            else if (layer->getType() == tmx::Layer::Type::Object)
            {
                const auto& objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
                for (const auto& obj : objects)
                {
                    sf::Vector2f position(obj.getPosition().x, obj.getPosition().y);
                    position *= 4.f; //scale up to world coords

                    //TODO objet data
                }
            }
        }

        m_topTexture.display();

        return true;
    }
    LOG("Failed opening map " + path, xy::Logger::Type::Error);
    return false;
}

const sf::Texture& MapLoader::getTopDownTexture() const
{
    return m_topTexture.getTexture();
}

const sf::Texture& MapLoader::getSideTexture() const
{
    return m_sideTexture.getTexture();
}