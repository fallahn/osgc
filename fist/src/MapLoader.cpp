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

#include "GameState.hpp"
#include "Sprite3D.hpp"
#include "GameConst.hpp"
#include "ResourceIDs.hpp"
#include "CommandIDs.hpp"
#include "RoomBuilder.hpp"
#include "WallData.hpp"
#include "ModelIO.hpp"
#include "SharedStateData.hpp"
#include "Collision.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include "xyginext/ecs/components/BroadPhaseComponent.hpp"

#include <xyginext/core/ConfigFile.hpp>

#include <SFML/OpenGL.hpp>

namespace
{
    struct WallProperties final
    {
        std::int32_t direction = -1;
        std::int32_t door = 0;
    };

    const float TriggerWidth = 192.f;
    const float TriggerHeight = 40.f;

    const std::array<sf::FloatRect, 4u> Hitboxes =
    {
        sf::FloatRect(-TriggerWidth / 2.f, -GameConst::RoomWidth / 2.f, TriggerWidth, TriggerHeight),
        sf::FloatRect((GameConst::RoomWidth / 2.f) - TriggerHeight, -TriggerWidth / 2.f, TriggerHeight, TriggerWidth),
        sf::FloatRect(-TriggerWidth / 2.f, (GameConst::RoomWidth / 2.f) - TriggerHeight, TriggerWidth, TriggerHeight),
        sf::FloatRect(-GameConst::RoomWidth / 2.f, -TriggerWidth / 2.f, TriggerHeight, TriggerWidth)
    };

    const float TargetOffset = (GameConst::RoomWidth / 2.f) + TriggerHeight + 128.f;
    const std::array<sf::Vector2f, 4u> Offsets =
    {
        sf::Vector2f(0.f, -TargetOffset),
        sf::Vector2f(TargetOffset, 0.f),
        sf::Vector2f(0.f, TargetOffset),
        sf::Vector2f(-TargetOffset, 0.f)
    };
}

bool GameState::loadMap()
{
    auto normalID = m_resources.load<sf::Texture>("assets/images/rooms/paper_normal.png");
    auto& normalTex = m_resources.get<sf::Texture>(normalID);
    normalTex.setRepeated(true);

    std::int32_t roomCount = 0;
    auto processRoom = [&](const xy::ConfigObject& room)
    {
        int roomID = -1;
        try
        {
            roomID = std::atoi(room.getId().c_str());
        }
        catch(...)
        {
            return;
        }

        RoomData roomData;
        roomData.id = roomID;

        auto roomPosition = calcRoomPosition(roomCount);

        const auto& properties = room.getProperties();
        if (properties.empty())
        {
            return;
        }

        //get room properties
        std::string textureName;
        for (const auto& prop : properties)
        {
            if (prop.getName() == "src")
            {
                textureName = prop.getValue<std::string>();
            }
            else if (prop.getName() == "ceiling")
            {
                roomData.hasCeiling = prop.getValue<std::int32_t>() != 0;
            }
            else if (prop.getName() == "itemID")
            {
                //TODO stash this somewhere
            }
            else if (prop.getName() == "sky_colour")
            {
                roomData.skyColour = prop.getValue<sf::Vector3f>();
            }
            else if (prop.getName() == "room_colour")
            {
                roomData.roomColour = prop.getValue<sf::Vector3f>();
            }
        }

        if (textureName.empty())
        {
            return;
        }

        //check if this room has walls
        const auto& objects = room.getObjects();
        std::array<WallProperties, 4u> walls = { };
        std::size_t wallIdx = 0;
        for (const auto& obj : objects)
        {
            if (obj.getName() == "wall" && wallIdx < walls.size())
            {
                const auto& wallProperties = obj.getProperties();
                for (const auto& wallProp : wallProperties)
                {
                    if (wallProp.getName() == "direction")
                    {
                        auto dir = wallProp.getValue<std::int32_t>();
                        if (dir >= 0 && dir < 4)
                        {
                            walls[wallIdx].direction = dir;
                        }
                    }
                    else if (wallProp.getName() == "door")
                    {
                        walls[wallIdx].door = wallProp.getValue<std::int32_t>();
                    }
                }

                wallIdx++;
            }
            else if (obj.getName() == "prop")
            {
                //parseModelNode(obj, roomPosition);
                float rotation = 0.f;
                sf::Vector2f position;
                std::string modelPath;

                const auto& properties = obj.getProperties();
                for (const auto& prop : properties)
                {
                    if (prop.getName() == "rotation")
                    {
                        rotation = prop.getValue<float>();
                    }
                    else if (prop.getName() == "position")
                    {
                        position = prop.getValue<sf::Vector2f>();
                    }
                    else if (prop.getName() == "model_src")
                    {
                        modelPath = prop.getValue<std::string>();
                    }
                }

                if (!modelPath.empty())
                {
                    xy::ConfigFile cfg;
                    cfg.loadFromFile(xy::FileSystem::getResourcePath() + modelPath);
                    if (cfg.getName() == "model")
                    {
                        auto entity = parseModelNode(cfg, modelPath);
                        if (entity.isValid())
                        {
                            auto targetPosition = position + roomPosition;
                            entity.getComponent<xy::Transform>().setPosition(targetPosition);
                            entity.getComponent<xy::Transform>().setRotation(rotation);

                            //transform AABB for broadphase
                            auto aabb = entity.getComponent<xy::Transform>().getTransform().transformRect(
                                entity.getComponent<xy::BroadphaseComponent>().getArea());

                            aabb.left -= targetPosition.x;
                            aabb.top -= targetPosition.y;
                            entity.getComponent<xy::BroadphaseComponent>().setArea(aabb);
                        }
                    }
                }
            }
        }
        std::sort(walls.begin(), walls.end(),
            [](const WallProperties& a, const WallProperties& b)
            {
                return a.direction < b.direction;
            });

        auto texID = m_resources.load<sf::Texture>(textureName);
        auto& tex = m_resources.get<sf::Texture>(texID);

        auto lightmapID = m_resources.load<sf::Texture>("assets/images/rooms/lightmaps/" + room.getId() + ".png");
        auto& lightmap = m_resources.get<sf::Texture>(lightmapID);

        //create the room entity
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(roomPosition);
        entity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
        entity.getComponent<xy::Drawable>().setTexture(&tex);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DWalls));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", normalTex);
        entity.getComponent<xy::Drawable>().bindUniform("u_lightmap", lightmap);
        entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::RoomHeight;
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        std::array<WallData, 4u> wallData = {};
        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        addFloor(verts);
        if (roomData.hasCeiling)
        {
            addCeiling(verts);
        }
        for (auto i = 0u; i < walls.size(); ++i)
        {
            if (walls[i].direction > -1)
            {
                addWall(verts, walls[i].direction);
                if (walls[i].door != 1)
                {
                    wallData[walls[i].direction].passable = false;
                }
            }
        }
        entity.getComponent<xy::Drawable>().updateLocalBounds();

        //add triggers to each side of the room
        auto position = entity.getComponent<xy::Transform>().getPosition();
        for (auto i = 0u; i < wallData.size(); ++i)
        {
            wallData[i].targetPoint = position + Offsets[i];

            auto triggerEnt = m_gameScene.createEntity();
            triggerEnt.addComponent<xy::Transform>();
            triggerEnt.addComponent<xy::BroadphaseComponent>().setArea(Hitboxes[i]);
            triggerEnt.addComponent<WallData>() = wallData[i];
            triggerEnt.addComponent<CollisionComponent>().id = CollisionID::Wall;
            entity.getComponent<xy::Transform>().addChild(triggerEnt.getComponent<xy::Transform>());
        }

        //stash room data
        m_mapData.roomData.push_back(roomData);

        roomCount++;
    };

    xy::ConfigFile map;
    if (map.loadFromFile(xy::FileSystem::getResourcePath() + "assets/game.map"))
    {
        const auto& objects = map.getObjects();
        for (const auto& obj : objects)
        {
            if (obj.getName() == "room")
            {
                processRoom(obj);
            }
        }

        std::sort(m_mapData.roomData.begin(), m_mapData.roomData.end(),
            [](const RoomData& a, const RoomData& b)
            {
                return a.id < b.id;
            });

        //TODO generate occlusion info from room data


        //set the initial lighting colours
        m_mapData.currentRoomColour = m_mapData.roomData[m_sharedData.currentRoom].roomColour;
        m_mapData.currentSkyColour = m_mapData.roomData[m_sharedData.currentRoom].skyColour;
        m_mapData.roomAmount = m_mapData.roomData[m_sharedData.currentRoom].hasCeiling ? 1.f : 0.f;
        m_mapData.skyAmount = m_mapData.roomData[m_sharedData.currentRoom].hasCeiling ? MapData::MinSkyAmount : 1.f;
        m_mapData.currentLightPosition = calcRoomPosition(m_sharedData.currentRoom);

        m_updateLighting = true;
        updateLighting();
        m_updateLighting = false;

        return roomCount == 64;
    }
    return false;
}

xy::Entity GameState::parseModelNode(const xy::ConfigObject& cfg, const std::string& absPath)
{
    std::string modelName = xy::FileSystem::getFileName(absPath);
    modelName = modelName.substr(0, modelName.find_last_of('.'));

    std::string binPath;
    std::string texture;
    float depth = 0.f;

    const auto& properties = cfg.getProperties();
    for (const auto& prop : properties)
    {
        const auto name = prop.getName();
        if (name == "bin")
        {
            binPath = prop.getValue<std::string>();
        }
        else if (name == "texture")
        {
            texture = prop.getValue<std::string>();
        }
        else if (name == "depth")
        {
            depth = prop.getValue<float>();
        }
    }

    if (!binPath.empty() && depth > 0)
    {
        auto rootPath = absPath.substr(0, absPath.find_last_of('/'));
        rootPath = rootPath.substr(rootPath.find("assets"));

        if (binPath[0] == '/')
        {
            binPath = binPath.substr(1);
        }

        if (m_modelVerts.count(modelName) == 0)
        {
            VertexCollection verts;
            readModelBinary(verts.vertices, xy::FileSystem::getResourcePath() + rootPath + "/" + binPath);

            if (!verts.vertices.empty())
            {
                m_modelVerts.insert(std::make_pair(modelName, verts));
            }
            else
            {
                xy::Logger::log("Unable to load " + modelName, xy::Logger::Type::Error);
            }
        }

        std::size_t texID = 0;
        if (!texture.empty())
        {
            texID = m_resources.load<sf::Texture>(rootPath + "/" + texture);
        }
        else
        {
            if (m_defaultTexID == 0)
            {
                m_defaultTexID = m_resources.load<sf::Texture>("assets/images/cam_debug.png");
            }
            texID = m_defaultTexID;
        }
        auto& tex = m_resources.get<sf::Texture>(texID);

        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>();
        entity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
        entity.getComponent<xy::Drawable>().setTexture(&tex);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().setCulled(false);
        entity.getComponent<xy::Drawable>().setPrimitiveType(sf::Triangles);
        entity.addComponent<Sprite3D>(m_matrixPool).depth = depth;
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        verts = m_modelVerts[modelName].vertices;
        //m_modelVerts[modelName].instanceCount++;
        entity.getComponent<xy::Drawable>().updateLocalBounds();

        //tex coords are normalised so we need to 'correct' this for sfml
        if (!texture.empty())
        {
            auto size = sf::Vector2f(tex.getSize());
            for (auto& v : verts)
            {
                v.texCoords.x *= size.x;
                v.texCoords.y *= size.y;
            }
        }

        //collision data
        entity.addComponent<xy::BroadphaseComponent>().setArea(entity.getComponent<xy::Drawable>().getLocalBounds());
        entity.addComponent<CollisionComponent>().id = CollisionID::Prop;

        return entity;
    }
    return {};
}