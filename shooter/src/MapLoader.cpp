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
#include "GameConsts.hpp"

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
    const sf::Vector2u SideMapSize(480,81);
    const float SandThickness = 16.f; //height of sand in side view
    const float SandThicknessScaled = SandThickness / 4.f;
}

MapLoader::MapLoader(const SpriteArray& sprites)
    : m_sprites(sprites)
{
    m_topBuffer.create(TopMapSize.x, TopMapSize.y);
    m_topTexture.create(TopMapSize.x, TopMapSize.y);
    m_sideTexture.create(SideMapSize.x, SideMapSize.y);
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

        tmx::ObjectGroup* objectLayer = nullptr;

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
                objectLayer = &layer->getLayerAs<tmx::ObjectGroup>();
            }
        }

        m_topTexture.display();


        //draw the side map if we found object data
        if (objectLayer)
        {
            m_sideTexture.clear();            
            
            std::vector<sf::Vertex> verts(12);
            verts[0].color = sf::Color::Cyan;
            verts[1] = { sf::Vector2f(xy::DefaultSceneSize.x, 0.f), sf::Color::Cyan };
            verts[2] = { sf::Vector2f(xy::DefaultSceneSize.x, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y), sf::Color::Cyan };
            verts[3] = { sf::Vector2f(0.f, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y), sf::Color::Cyan };

            verts[4] = { sf::Vector2f(0.f, (ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y) - SandThickness), sf::Color::Yellow };
            verts[5] = { sf::Vector2f(xy::DefaultSceneSize.x, (ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y) - SandThickness), sf::Color::Yellow };
            verts[6] = { sf::Vector2f(xy::DefaultSceneSize.x, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y), sf::Color::Yellow };
            verts[7] = { sf::Vector2f(0.f, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y), sf::Color::Yellow };
            
            verts[8] = { sf::Vector2f(0.f, (ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y) - 4.f), sf::Color::Black };
            verts[9] = { sf::Vector2f(xy::DefaultSceneSize.x, (ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y) - 4.f), sf::Color::Black };
            verts[10] = { sf::Vector2f(xy::DefaultSceneSize.x, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y), sf::Color::Black };
            verts[11] = { sf::Vector2f(0.f, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y), sf::Color::Black };
            
            for (auto& v : verts) v.position /= 4.f;

            m_sideTexture.draw(verts.data(), verts.size(), sf::Quads);

            auto objects = objectLayer->getObjects();
            //sort by x pos so drawn in correct order
            std::sort(objects.begin(), objects.end(), [](const tmx::Object& objA, const tmx::Object& objB) {return objA.getPosition().x > objB.getPosition().x; });

            for (const auto& obj : objects)
            {
                sf::Vector2f position(obj.getPosition().x, obj.getPosition().y);
                position /= 2.f; //side map is half map scale, then texture upscaled (boy this is complicated)

                auto objBounds = obj.getAABB();
                objBounds.left /= 2.f;
                objBounds.top /= 2.f;
                objBounds.width /= 2.f;
                objBounds.height /= 2.f;

                const auto type = obj.getType();
                if (type == "building")
                {
                    auto leftBounds = m_sprites[SpriteID::BuildingLeft].getTextureRect();
                    auto rightBounds = m_sprites[SpriteID::BuildingRight].getTextureRect();
                    verts.resize(12);
                    verts[0] = { sf::Vector2f(0.f, 0.f), sf::Vector2f(leftBounds.left, leftBounds.top) };
                    verts[1] = { sf::Vector2f(leftBounds.width, 0.f), sf::Vector2f(leftBounds.left + leftBounds.width, leftBounds.top) };
                    verts[2] = { sf::Vector2f(leftBounds.width, leftBounds.height), sf::Vector2f(leftBounds.left + leftBounds.width, leftBounds.top + leftBounds.height) };
                    verts[3] = { sf::Vector2f(0.f, leftBounds.height), sf::Vector2f(leftBounds.left, leftBounds.top + leftBounds.height) };

                    auto bounds = m_sprites[SpriteID::BuildingCentre].getTextureRect();
                    verts[4] = { sf::Vector2f(leftBounds.width, 0.f), sf::Vector2f(bounds.left, bounds.top) };
                    verts[5] = { sf::Vector2f(objBounds.height - rightBounds.width, 0.f), sf::Vector2f(bounds.left + bounds.width, bounds.top) };
                    verts[6] = { sf::Vector2f(objBounds.height - rightBounds.width, bounds.height), sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height) };
                    verts[7] = { sf::Vector2f(leftBounds.width, bounds.height), sf::Vector2f(bounds.left, bounds.top + bounds.height) };
                    
                    verts[8] = { sf::Vector2f(objBounds.height - rightBounds.width, 0.f), sf::Vector2f(rightBounds.left, rightBounds.top) };
                    verts[9] = { sf::Vector2f(objBounds.height, 0.f), sf::Vector2f(rightBounds.left + rightBounds.width, rightBounds.top) };
                    verts[10] = { sf::Vector2f(objBounds.height, rightBounds.height), sf::Vector2f(rightBounds.left + rightBounds.width, rightBounds.top + rightBounds.height) };
                    verts[11] = { sf::Vector2f(objBounds.height - rightBounds.width, rightBounds.height), sf::Vector2f(rightBounds.left, rightBounds.top + rightBounds.height) };

                    sf::RenderStates states;
                    states.transform.translate(position.y, SideMapSize.y - (bounds.height + SandThicknessScaled));
                    states.texture = m_sprites[SpriteID::BuildingLeft].getTexture();

                    m_sideTexture.draw(verts.data(), verts.size(), sf::Quads, states);
                }
                else if (type == "barrier")
                {
                    auto leftBounds = m_sprites[SpriteID::HillLeftWide].getTextureRect();
                    auto rightBounds = m_sprites[SpriteID::HillRightWide].getTextureRect();
                    verts.resize(12);
                    verts[0] = { sf::Vector2f(0.f, 0.f), sf::Vector2f(leftBounds.left, leftBounds.top) };
                    verts[1] = { sf::Vector2f(leftBounds.width, 0.f), sf::Vector2f(leftBounds.left + leftBounds.width, leftBounds.top) };
                    verts[2] = { sf::Vector2f(leftBounds.width, leftBounds.height), sf::Vector2f(leftBounds.left + leftBounds.width, leftBounds.top + leftBounds.height) };
                    verts[3] = { sf::Vector2f(0.f, leftBounds.height), sf::Vector2f(leftBounds.left, leftBounds.top + leftBounds.height) };

                    auto bounds = m_sprites[SpriteID::HillCentre].getTextureRect();
                    verts[4] = { sf::Vector2f(leftBounds.width, (leftBounds.height - bounds.height)), sf::Vector2f(bounds.left, bounds.top) };
                    verts[5] = { sf::Vector2f(objBounds.height - rightBounds.width, (leftBounds.height - bounds.height)), sf::Vector2f(bounds.left + bounds.width, bounds.top) };
                    verts[6] = { sf::Vector2f(objBounds.height - rightBounds.width, leftBounds.height), sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height) };
                    verts[7] = { sf::Vector2f(leftBounds.width, leftBounds.height), sf::Vector2f(bounds.left, bounds.top + bounds.height) };

                    verts[8] = { sf::Vector2f(objBounds.height - rightBounds.width, 0.f), sf::Vector2f(rightBounds.left, rightBounds.top) };
                    verts[9] = { sf::Vector2f(objBounds.height, 0.f), sf::Vector2f(rightBounds.left + rightBounds.width, rightBounds.top) };
                    verts[10] = { sf::Vector2f(objBounds.height, rightBounds.height), sf::Vector2f(rightBounds.left + rightBounds.width, rightBounds.top + rightBounds.height) };
                    verts[11] = { sf::Vector2f(objBounds.height - rightBounds.width, rightBounds.height), sf::Vector2f(rightBounds.left, rightBounds.top + rightBounds.height) };

                    sf::RenderStates states;
                    states.transform.translate(position.y, SideMapSize.y - (leftBounds.height + SandThicknessScaled));
                    states.texture = m_sprites[SpriteID::HillLeftWide].getTexture();

                    m_sideTexture.draw(verts.data(), verts.size(), sf::Quads, states);
                }
                else if (type == "tree")
                {
                    //trees are fixed size so draw sprite directly
                    auto bounds = m_sprites[SpriteID::TreeIcon].getTextureRect();
                    verts.resize(4);
                    verts[0] = { sf::Vector2f(-bounds.width / 2.f, 0.f), sf::Vector2f(bounds.left, bounds.top) };
                    verts[1] = { sf::Vector2f(bounds.width / 2.f, 0.f), sf::Vector2f(bounds.left + bounds.width, bounds.top) };
                    verts[2] = { sf::Vector2f(bounds.width / 2.f, bounds.height), sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height) };
                    verts[3] = { sf::Vector2f(-bounds.width / 2.f, bounds.height), sf::Vector2f(bounds.left, bounds.top + bounds.height) };

                    sf::RenderStates states;
                    states.transform.translate(position.y + (bounds. height / 2.f), SideMapSize.y - (bounds.height + SandThicknessScaled));
                    states.texture = m_sprites[SpriteID::TreeIcon].getTexture();

                    m_sideTexture.draw(verts.data(), verts.size(), sf::Quads, states);

                }
                else if (type == "bush")
                {
                    //TODO refactor to share tree code, above
                    //TODO use sf::Sprite / texture rect rather than vert array?
                    auto bounds = m_sprites[SpriteID::BushIcon].getTextureRect();
                    verts.resize(4);
                    verts[0] = { sf::Vector2f(-bounds.width / 2.f, 0.f), sf::Vector2f(bounds.left, bounds.top) };
                    verts[1] = { sf::Vector2f(bounds.width / 2.f, 0.f), sf::Vector2f(bounds.left + bounds.width, bounds.top) };
                    verts[2] = { sf::Vector2f(bounds.width / 2.f, bounds.height), sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height) };
                    verts[3] = { sf::Vector2f(-bounds.width / 2.f, bounds.height), sf::Vector2f(bounds.left, bounds.top + bounds.height) };

                    sf::RenderStates states;
                    states.transform.translate(position.y + (bounds.height / 2.f), SideMapSize.y - (bounds.height + SandThicknessScaled));
                    states.texture = m_sprites[SpriteID::BushIcon].getTexture();

                    m_sideTexture.draw(verts.data(), verts.size(), sf::Quads, states);

                }
            }
            m_sideTexture.display();
        }
        else
        {
            xy::Logger::log("Missing object layer");
            return false;
        }

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

void MapLoader::renderSprite(std::int32_t spriteID, sf::Vector2f position)
{
    //TODO since SFML switched its sprite implementation to vertex buffers
    //creating in-place sprites like this is probably just a good way to waste
    //GPU memory.

    auto bounds = m_sprites[spriteID].getTextureRect();
    sf::Sprite spr(*m_sprites[spriteID].getTexture());
    spr.setTextureRect(sf::IntRect(bounds));
    spr.setPosition(position / 4.f);
    //TODO this is a fudge as we're only drawinf craters
    //would the sprite sheet format benefit from an origin property?
    spr.setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    m_topBuffer.clear();
    m_topBuffer.draw(sf::Sprite(m_topTexture.getTexture()));
    m_topBuffer.draw(spr);
    m_topBuffer.display();

    m_topTexture.clear();
    m_topTexture.draw(sf::Sprite(m_topBuffer.getTexture()));
    m_topTexture.display();
}