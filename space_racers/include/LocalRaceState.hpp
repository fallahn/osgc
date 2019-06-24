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

#pragma once

#include "StateIDs.hpp"
#include "MapParser.hpp"
#include "RenderPath.hpp"
#include "ResourceIDs.hpp"
#include "MatrixPool.hpp"
#include "InputParser.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/audio/AudioScape.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Clock.hpp>

class LocalRaceState final : public xy::State 
{
public:
    LocalRaceState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override { return StateID::LocalRace; }

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
    xy::AudioScape m_raceSounds;

    MapParser m_mapParser;
    RenderPath m_renderPath;
    MatrixPool m_matrixPool;

    std::array<InputParser, 4u> m_playerInputs;

    sf::Clock m_stateTimer;
    enum
    {
        Readying,
        Counting,
        Racing
    }m_state;

    void initScene();
    void loadResources();

    bool loadMap();
    void createRoids();
    void buildUI();
    void addLapPoint(xy::Entity, sf::Color);
    void spawnVehicle();
    void spawnTrail(xy::Entity, sf::Color);

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};