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
    const float TriggerHeight = 20.f;

    const std::array<sf::FloatRect, 4u> Hitboxes =
    {
        sf::FloatRect(-TriggerWidth / 2.f, -GameConst::RoomWidth / 2.f, TriggerWidth, TriggerHeight),
        sf::FloatRect((GameConst::RoomWidth / 2.f) - TriggerHeight, -TriggerWidth / 2.f, TriggerHeight, TriggerWidth),
        sf::FloatRect(-TriggerWidth / 2.f, (GameConst::RoomWidth / 2.f) - TriggerHeight, TriggerWidth, TriggerHeight),
        sf::FloatRect(-GameConst::RoomWidth / 2.f, -TriggerWidth / 2.f, TriggerHeight, TriggerWidth)
    };

    const float TargetOffset = (GameConst::RoomWidth / 2.f) + TriggerHeight + 72.f;
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
        const auto& properties = room.getProperties();
        if (properties.empty())
        {
            return;
        }

        //get room properties
        std::string textureName;
        bool hasCeiling = false;
        for (const auto& prop : properties)
        {
            if (prop.getName() == "src")
            {
                textureName = prop.getValue<std::string>();
            }
            else if (prop.getName() == "ceiling")
            {
                hasCeiling = prop.getValue<std::int32_t>() != 0;
            }
            else if (prop.getName() == "itemID")
            {
                //TODO stash this somewhere
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
            if (obj.getName() == "wall")
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
            if (wallIdx == walls.size())
            {
                break;
            }
        }
        std::sort(walls.begin(), walls.end(),
            [](const WallProperties& a, const WallProperties& b)
            {
                return a.direction < b.direction;
            });

        auto texID = m_resources.load<sf::Texture>(textureName);
        auto& tex = m_resources.get<sf::Texture>(texID);

        //create the room entity
        auto xCoord = roomCount % GameConst::RoomsPerRow;
        auto yCoord = roomCount / GameConst::RoomsPerRow;

        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xCoord * (GameConst::RoomWidth + GameConst::RoomPadding), 
                                                        yCoord * (GameConst::RoomWidth + GameConst::RoomPadding));
        entity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
        entity.getComponent<xy::Drawable>().setTexture(&tex);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DWalls));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", normalTex);
        entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::RoomHeight;
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        std::array<WallData, 4u> wallData = {};
        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        addFloor(verts);
        if (hasCeiling)
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
            entity.getComponent<xy::Transform>().addChild(triggerEnt.getComponent<xy::Transform>());
        }

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

        return roomCount == 64;
    }
    return false;
}