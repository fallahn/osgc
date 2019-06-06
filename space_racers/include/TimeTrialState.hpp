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

#include "StateIDs.hpp"
#include "MapParser.hpp"
#include "RenderPath.hpp"
#include "ResourceIDs.hpp"
#include "MatrixPool.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/audio/AudioScape.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <SFML/Graphics/RenderTexture.hpp>

class TimeTrialState final : public xy::State
{
public:
    TimeTrialState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override { return StateID::TimeTrial; }

private:
    SharedData& m_sharedData;
    xy::Scene m_backgroundScene;
    xy::Scene m_gameScene;
    xy::Scene m_uiScene;

    std::array<std::size_t, TextureID::Game::Count> m_textureIDs;
    std::array<xy::Sprite, SpriteID::Game::Count> m_sprites;
    std::array<sf::RenderTexture, 2u> m_trackTextures;
    xy::ResourceHandler m_resources;
    xy::ShaderResource m_shaders;

    xy::AudioResource m_audioResource;
    xy::AudioScape m_uiSounds;

    MapParser m_mapParser;
    RenderPath m_renderPath;
    MatrixPool m_matrixPool;

    void initScene();
    void loadResources();

    bool loadMap();
    void addProps();
    void buildUI();
    void spawnVehicle();
    void spawnTrail(xy::Entity, sf::Color);

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};