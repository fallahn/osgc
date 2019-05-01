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
#include "CollisionTypes.hpp"

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

    const float BuildingHeight = 46.f;
    const float TreeHeight = 58.f;
    const float BushHeight = 24.f;
    const float BarrierHeight = 18.f;
    const float SandbagHeight = 12.f;

    template <typename T>
    tmx::Rectangle<T>& operator *= (tmx::Rectangle<T>& l, T r)
    {
        l.left *= r;
        l.top *= r;
        l.width *= r;
        l.height *= r;
        return l;
    }
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
    clearObjectData();

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
        tmx::ObjectGroup* navigationLayer = nullptr;
        tmx::ObjectGroup* spawnLayer = nullptr;

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
                if (layer->getName() == "navigation")
                {
                    navigationLayer = &layer->getLayerAs<tmx::ObjectGroup>();
                }
                else if (layer->getName() == "objects")
                {
                    objectLayer = &layer->getLayerAs<tmx::ObjectGroup>();
                }
                else if (layer->getName() == "spawn")
                {
                    spawnLayer = &layer->getLayerAs<tmx::ObjectGroup>();
                }
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

            auto drawSprite = [&](const xy::Sprite& spr, sf::Vector2f position)
            {
                auto bounds = spr.getTextureRect();
                verts.resize(4);
                verts[0] = { sf::Vector2f(-bounds.width / 2.f, 0.f), sf::Vector2f(bounds.left, bounds.top) };
                verts[1] = { sf::Vector2f(bounds.width / 2.f, 0.f), sf::Vector2f(bounds.left + bounds.width, bounds.top) };
                verts[2] = { sf::Vector2f(bounds.width / 2.f, bounds.height), sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height) };
                verts[3] = { sf::Vector2f(-bounds.width / 2.f, bounds.height), sf::Vector2f(bounds.left, bounds.top + bounds.height) };

                sf::RenderStates states;
                states.transform.translate(position.y + (bounds.height / 2.f), SideMapSize.y - (bounds.height + SandThicknessScaled));
                states.texture = spr.getTexture();

                m_sideTexture.draw(verts.data(), verts.size(), sf::Quads, states);
            };

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

                    //stash the collion box
                    objBounds *= 8.f;
                    CollisionBox cb;
                    cb.worldBounds = { objBounds.left, objBounds.top, objBounds.width, objBounds.height };
                    cb.type = CollisionBox::Building;
                    cb.height = BuildingHeight;
                    m_collisionBoxes.push_back(cb);

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

                    objBounds *= 8.f;
                    CollisionBox cb;
                    cb.worldBounds = { objBounds.left, objBounds.top, objBounds.width, objBounds.height };
                    cb.type = CollisionBox::Structure;
                    cb.height = BarrierHeight;
                    m_collisionBoxes.push_back(cb);
                }
                else if (type == "tree")
                {
                    //trees are fixed size so draw sprite directly
                    drawSprite(m_sprites[SpriteID::TreeIcon], position);

                    objBounds *= 8.f;
                    CollisionBox cb;
                    cb.worldBounds = { objBounds.left, objBounds.top, objBounds.width, objBounds.height };
                    cb.type = CollisionBox::Structure;
                    cb.filter = CollisionBox::NoDecal | CollisionBox::Solid;
                    cb.height = TreeHeight;
                    m_collisionBoxes.push_back(cb);

                }
                else if (type == "bush")
                {
                    drawSprite(m_sprites[SpriteID::BushIcon], position);

                    objBounds *= 8.f;
                    CollisionBox cb;
                    cb.worldBounds = { objBounds.left, objBounds.top, objBounds.width, objBounds.height };
                    cb.type = CollisionBox::Structure;
                    cb.height = BushHeight;
                    m_collisionBoxes.push_back(cb);

                }
                else if (type == "water")
                {
                    objBounds *= 8.f;
                    CollisionBox cb;
                    cb.worldBounds = { objBounds.left, objBounds.top, objBounds.width, objBounds.height };
                    cb.type = CollisionBox::Water;
                    cb.filter = CollisionBox::NoDecal | CollisionBox::Solid;
                    m_collisionBoxes.push_back(cb);
                }
                else if (type == "sandbag")
                {
                    objBounds *= 8.f;
                    CollisionBox cb;
                    cb.worldBounds = { objBounds.left, objBounds.top, objBounds.width, objBounds.height };
                    cb.type = CollisionBox::Structure;
                    cb.filter = CollisionBox::NoDecal | CollisionBox::Solid;
                    cb.height = SandbagHeight;
                    m_collisionBoxes.push_back(cb);
                }
                else if (type == "border")
                {
                    objBounds *= 8.f;
                    CollisionBox cb;
                    cb.worldBounds = { objBounds.left, objBounds.top, objBounds.width, objBounds.height };
                    cb.type = CollisionBox::Structure;
                    cb.filter = CollisionBox::NoDecal | CollisionBox::Solid;
                    cb.height = SandbagHeight;
                    m_collisionBoxes.push_back(cb);
                }
            }
            m_sideTexture.display();
        }
        else
        {
            xy::Logger::log("Missing object layer");
            return false;
        }

        if (navigationLayer)
        {
            const auto& objs = navigationLayer->getObjects();
            for (const auto& obj : objs)
            {
                if (obj.getShape() == tmx::Object::Shape::Polyline)
                {
                    const auto& points = obj.getPoints();

                    if (points.size() > 1)
                    {
                        auto pos = obj.getPosition() * 4.f;
                        std::vector<sf::Vector2f> path;
                        for (const auto& p : points)
                        {
                            path.emplace_back(pos.x + (p.x * 4.f), pos.y + (p.y * 4.f));
                        }
                        m_navigationNodes.push_back(path);
                    }
                }
            }
        }
        else
        {
            xy::Logger::log("Missing navigation layer");
            return false;
        }

        if (spawnLayer)
        {
            const auto& objs = spawnLayer->getObjects();
            for (const auto& obj : objs)
            {
                if (obj.getShape() == tmx::Object::Shape::Point)
                {
                    sf::Vector2f position(obj.getPosition().x, obj.getPosition().y);
                    position *= 4.f;

                    std::int32_t type = -1;
                    if (obj.getType() == "alien spawn")
                    {
                        type = SpawnType::Alien;
                    }
                    else if (obj.getType() == "human spawn")
                    {
                        type = SpawnType::Human;
                    }
                    
                    if (type > -1)
                    {
                        m_spawnPoints.emplace_back(std::make_pair(position, type));
                    }
                }
            }
        }
        else
        {
            xy::Logger::log("no spawn layer found", xy::Logger::Type::Error);
            return false;
        }

        if (m_collisionBoxes.empty() || m_navigationNodes.empty() || m_spawnPoints.empty())
        {
            xy::Logger::log("missing some map data...", xy::Logger::Type::Error);
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

void MapLoader::renderSprite(std::int32_t spriteID, sf::Vector2f position, float rotation)
{
    //TODO since SFML switched its sprite implementation to vertex buffers
    //creating in-place sprites like this is probably just a good way to waste
    //GPU memory.
    //maybe we could cache these?

    auto bounds = m_sprites[spriteID].getTextureRect();
    sf::Sprite spr(*m_sprites[spriteID].getTexture());
    spr.setTextureRect(sf::IntRect(bounds));
    spr.setPosition(position / 4.f);
    //TODO this is a fudge as we're assuming all sprites have a centre origin
    spr.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    spr.setRotation(rotation);

    m_topBuffer.clear();
    m_topBuffer.draw(sf::Sprite(m_topTexture.getTexture()));
    m_topBuffer.draw(spr);
    m_topBuffer.display();

    m_topTexture.clear();
    m_topTexture.draw(sf::Sprite(m_topBuffer.getTexture()));
    m_topTexture.display();
}

const std::vector<CollisionBox>& MapLoader::getCollisionBoxes() const
{
    return m_collisionBoxes;
}

const std::vector<std::vector<sf::Vector2f>>& MapLoader::getNavigationNodes() const
{
    return m_navigationNodes;
}

const std::vector<std::pair<sf::Vector2f, std::int32_t>>& MapLoader::getSpawnPoints() const
{
    return m_spawnPoints;
}

void MapLoader::clearObjectData()
{
    m_collisionBoxes.clear();
    m_navigationNodes.clear();
    m_spawnPoints.clear();
}