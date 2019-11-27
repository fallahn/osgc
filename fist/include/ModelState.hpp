/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#pragma once

#include "OBJ_Loader.h"
#include "MatrixPool.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/core/ConfigFile.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/gui/GuiClient.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>

#include <string>
#include <map>

struct SharedData;
class ModelState final : public xy::State, public xy::GuiClient
{
public:
    ModelState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:
    SharedData& m_sharedData;
    xy::Scene m_uiScene;
    xy::Entity m_roomEntity;
    sf::Texture m_roomTexture;
    xy::ConfigFile m_mapData;
    std::vector<xy::ConfigObject*> m_roomList;

    objl::Loader m_objLoader;
    bool m_modelLoaded;
    std::string m_modelInfo;
    std::string m_modelTextureName;
    std::size_t m_defaultTexID;

    bool m_showModelImporter;
    bool m_showAerialView;
    bool m_quitEditor;
    bool m_loadModel;
    bool m_saveRoom;
    std::int32_t m_currentView;
    xy::Entity m_selectedModel;

    xy::ResourceHandler m_resources;
    xy::ShaderResource m_shaders;
    MatrixPool m_matrixPool;

    struct VertexCollection final
    {
        std::vector<sf::Vertex> vertices;
        std::int32_t instanceCount = 0;
    };

    std::map<std::string, VertexCollection> m_modelVerts;
    std::map<std::string, xy::Entity> m_modelList;
    static constexpr std::size_t MaxModels = 8;

    sf::RenderTexture m_aerialPreview;
    sf::Sprite m_aerialSprite;
    std::vector<sf::RectangleShape> m_aerialObjects;
    sf::RectangleShape m_aerialBackground;
    sf::CircleShape m_eyeconOuter;

    std::uint16_t m_wallFlags;
    std::vector<sf::RectangleShape> m_aerialWalls;

    sf::Vector3f m_skyColour;
    sf::Vector3f m_roomColour;

    void initScene();
    void parseVerts();
    bool exportModel(const std::string&);

    void loadModel(const std::string&);
    xy::Entity parseModelNode(const xy::ConfigObject&, const std::string&);

    void setCamera(std::int32_t);
    void setRoomGeometry();

    void saveRoom();
    void applyLightingToAll();
};