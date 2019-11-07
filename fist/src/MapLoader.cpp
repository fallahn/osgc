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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/core/ConfigFile.hpp>

#include <SFML/OpenGL.hpp>

namespace
{
    enum Direction
    {
        Nort,Eas,Sout,Wes //these names clash with something windowsy 0.o
    };

    const sf::FloatRect NorthUV(540.f, 0.f, 960.f, 540.f);
    const sf::FloatRect EastUV(2040.f, 540.f, -540.f, 960.f);
    const sf::FloatRect SouthUV(1500.f, 2040.f, -960.f, -540.f);
    const sf::FloatRect WestUV(0.f, 1500.f, 540.f, -960.f);
    const sf::FloatRect FloorUV(540.f, 540.f, 960.f, 960.f);
    const sf::FloatRect CeilingUV(2040.f, 540.f, 960.f, 960.f);

    void addFloor(std::vector<sf::Vertex>& verts)
    {
        std::vector<sf::Vertex> temp;

        sf::Vertex vert;
        vert.color.a = 0;
        //put normal data in colour rgb
        vert.color.r = 127;
        vert.color.g = 127;
        vert.color.b = 255;

        vert.position = { -GameConst::RoomWidth, -GameConst::RoomWidth };
        vert.position /= 2.f;

        vert.texCoords = { FloorUV.left, FloorUV.top };
        temp.push_back(vert);

        vert.position.x += GameConst::RoomWidth;
        vert.texCoords.x += FloorUV.width;
        temp.push_back(vert);

        vert.position.y += GameConst::RoomWidth;
        vert.texCoords.y += FloorUV.height;
        temp.push_back(vert);

        vert.position.x -= GameConst::RoomWidth;
        vert.texCoords.x -= FloorUV.width;
        temp.push_back(vert);

        std::reverse(temp.begin(), temp.end());
        verts.insert(verts.end(), temp.begin(), temp.end());
    }

    void addCeiling(std::vector<sf::Vertex>& verts)
    {
        //using this to reverse the winding because I cba to manually rearrange the code
        std::vector<sf::Vertex> temp;

        sf::Vertex vert;
        vert.color.a = 255;
        //put normal data in colour rgb
        vert.color.r = 127;
        vert.color.g = 127;
        vert.color.b = 0;

        vert.position = { GameConst::RoomWidth, -GameConst::RoomWidth };
        vert.position /= 2.f;

        vert.texCoords = { CeilingUV.left, CeilingUV.top };
        temp.push_back(vert);

        vert.position.x -= GameConst::RoomWidth;
        vert.texCoords.x += CeilingUV.width;
        temp.push_back(vert);

        vert.position.y += GameConst::RoomWidth;
        vert.texCoords.y += CeilingUV.height;
        temp.push_back(vert);

        vert.position.x += GameConst::RoomWidth;
        vert.texCoords.x -= CeilingUV.width;
        temp.push_back(vert);

        std::reverse(temp.begin(), temp.end());
        verts.insert(verts.end(), temp.begin(), temp.end());
    }

    void addWall(std::vector<sf::Vertex>& verts, std::int32_t dir)
    {
        std::vector<sf::Vertex> temp;
        switch (dir)
        {
        default: return;
        case Nort:
        {
            sf::Vertex vert;
            vert.color.a = 255;
            //put normal data into colour property
            vert.color.r = 127;
            vert.color.g = 255;
            vert.color.b = 127;

            vert.position = { -GameConst::RoomWidth, -GameConst::RoomWidth };
            vert.position /= 2.f;

            vert.texCoords = { NorthUV.left, NorthUV.top };
            temp.push_back(vert);

            vert.position.x += GameConst::RoomWidth;
            vert.texCoords.x += NorthUV.width;
            temp.push_back(vert);

            vert.color.a = 0;
            vert.texCoords.y += NorthUV.height;
            temp.push_back(vert);

            vert.position.x -= GameConst::RoomWidth;
            vert.texCoords.x -= NorthUV.width;
            temp.push_back(vert);
        }
            break;
        case Sout:
        {
            sf::Vertex vert;
            vert.color.a = 255;
            //put normal data into colour property
            vert.color.r = 127;
            vert.color.g = 0;
            vert.color.b = 127;

            vert.position = { GameConst::RoomWidth, GameConst::RoomWidth };
            vert.position /= 2.f;

            vert.texCoords = { SouthUV.left, SouthUV.top };
            temp.push_back(vert);

            vert.position.x -= GameConst::RoomWidth;
            vert.texCoords.x += SouthUV.width;
            temp.push_back(vert);

            vert.color.a = 0;
            vert.texCoords.y += SouthUV.height;
            temp.push_back(vert);

            vert.position.x += GameConst::RoomWidth;
            vert.texCoords.x -= SouthUV.width;
            temp.push_back(vert);
        }
            break;
        case Eas:
        {
            sf::Vertex vert;
            vert.color.a = 255;
            //put normal data into colour property
            vert.color.r = 0;
            vert.color.g = 127;
            vert.color.b = 127;

            vert.position = { GameConst::RoomWidth, -GameConst::RoomWidth };
            vert.position /= 2.f;

            vert.texCoords = { EastUV.left, EastUV.top };
            temp.push_back(vert);

            vert.position.y += GameConst::RoomWidth;
            vert.texCoords.y += EastUV.height;
            temp.push_back(vert);

            vert.color.a = 0;
            vert.texCoords.x += EastUV.width;
            temp.push_back(vert);

            vert.position.y -= GameConst::RoomWidth;
            vert.texCoords.y -= EastUV.height;
            temp.push_back(vert);
        }
            break;
        case Wes:
        {
            sf::Vertex vert;
            vert.color.a = 255;
            //put normal data into colour property
            vert.color.r = 255;
            vert.color.g = 127;
            vert.color.b = 127;

            vert.position = { -GameConst::RoomWidth, GameConst::RoomWidth };
            vert.position /= 2.f;

            vert.texCoords = { WestUV.left, WestUV.top };
            temp.push_back(vert);

            vert.position.y -= GameConst::RoomWidth;
            vert.texCoords.y += WestUV.height;
            temp.push_back(vert);

            vert.color.a = 0;
            vert.texCoords.x += WestUV.width;
            temp.push_back(vert);

            vert.position.y += GameConst::RoomWidth;
            vert.texCoords.y -= WestUV.height;
            temp.push_back(vert);
        }
            break;
        }

        std::reverse(temp.begin(), temp.end());
        verts.insert(verts.end(), temp.begin(), temp.end());
    }
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
        std::array<bool, 4u> walls = { false,false,false,false };
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
                            walls[dir] = true;
                        }
                    }
                    //TODO look up door properties
                }
            }
        }

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

        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        addFloor(verts);
        if (hasCeiling)
        {
            addCeiling(verts);
        }
        for (auto i = 0u; i < walls.size(); ++i)
        {
            if (walls[i])
            {
                addWall(verts, i);
            }
        }
        entity.getComponent<xy::Drawable>().updateLocalBounds();

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