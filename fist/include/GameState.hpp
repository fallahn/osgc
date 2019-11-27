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

#include "CameraInput.hpp"
#include "MatrixPool.hpp"
#include "WallData.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/gui/GuiClient.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <map>

struct SharedData;

class GameState final : public xy::State, public xy::GuiClient
{
public:
    GameState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:
    SharedData& m_sharedData;
    xy::Scene m_gameScene;
    xy::Scene m_uiScene;
    MatrixPool m_matrixPool;

    xy::ResourceHandler m_resources;
    xy::ShaderResource m_shaders;
    std::size_t m_defaultTexID;
    std::array<std::size_t, TextureID::Count> m_textureIDs;

    struct VertexCollection final
    {
        std::vector<sf::Vertex> vertices;
        std::int32_t instanceCount = 0;
    };

    std::map<std::string, VertexCollection> m_modelVerts;

    MapData m_mapData;
    bool m_cameraUnlocked;
    bool m_updateLighting;

    void initScene();
    void loadResources();
    bool loadMap();
    void addPlayer();
    void buildUI();

    xy::Entity parseModelNode(const xy::ConfigObject&, const std::string&);
    void updateLighting();
#ifdef XY_DEBUG
    CameraInput m_cameraInput;
    void debugSetup();
#endif
};