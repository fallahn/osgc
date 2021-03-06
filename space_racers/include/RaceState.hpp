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
#include "ResourceIDs.hpp"
#include "InputParser.hpp"
#include "MapParser.hpp"
#include "MatrixPool.hpp"
#include "RenderPath.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/ParticleEmitter.hpp>
#include <xyginext/audio/AudioScape.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include <vector>
#include <array>

struct VehicleData;
struct ActorData;
struct VehicleActorUpdate;
struct ClientUpdate;
class RaceState final : public xy::State
{
public:
    RaceState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Race; }

private:
    SharedData& m_sharedData;

    xy::Scene m_backgroundScene;
    xy::Scene m_gameScene;
    xy::Scene m_uiScene;

    xy::AudioResource m_audioResource;
    xy::AudioScape m_uiSounds;
    xy::AudioScape m_raceSounds;
    
    MapParser m_mapParser;
    xy::ResourceHolder m_resources;
    std::array<std::size_t, TextureID::Game::Count> m_textureIDs;

    xy::ShaderResource m_shaders;
    std::array<sf::RenderTexture, 2u> m_trackTextures;

    RenderPath m_renderPath;

    MatrixPool m_matrixPool;
    std::array<xy::Sprite, SpriteID::Game::Count> m_sprites;

    InputParser m_playerInput;
    sf::Clock m_pingClock;

    xy::EmitterSettings m_skidSettings;

    void initScene();
    void loadResources();
    void buildWorld();
    void addProps();
    void buildUI();
    void addLapPoint(xy::Entity, sf::Color);

    void buildTest();

    void handlePackets();

    void spawnVehicle(const VehicleData&);
    void spawnActor(const ActorData&);
    void updateActor(const VehicleActorUpdate&);
    void reconcile(const ClientUpdate&);

    void resetNetVehicle(const VehicleData&);
    void explodeNetVehicle(std::uint32_t);
    void fallNetVehicle(std::uint32_t);
    void removeNetVehicle(std::uint32_t);

    void showTimer();
    void spawnTrail(xy::Entity, sf::Color);
    void updateLapLine(std::uint32_t);

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};