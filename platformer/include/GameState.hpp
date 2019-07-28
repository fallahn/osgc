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
#include "MapLoader.hpp"
#include "InputParser.hpp"
#include "AnimationMap.hpp"

#include <xyginext/audio/AudioScape.hpp>
#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/ParticleEmitter.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/resources/Resource.hpp>

#include <SFML/Graphics/RenderTexture.hpp>

struct SharedData;
class GameState final : public xy::State
{
public:
    GameState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Game; }

private:
    xy::Scene m_tilemapScene;
    xy::Scene m_gameScene;
    xy::Scene m_uiScene;
    SharedData& m_sharedData;
    xy::ResourceHandler m_resources;
    xy::ShaderResource m_shaders;

    sf::RenderTexture m_tilemapBuffer;

    std::array<std::size_t, TextureID::Game::Count> m_textureIDs;
    SpriteArray<SpriteID::GearBoy::Count> m_sprites;
    AnimationMap<AnimID::Player::Count> m_playerAnimations;
    AnimationMap<AnimID::Checkpoint::Count> m_checkpointAnimations;
    AnimationMap<AnimID::Enemy::Count> m_crawlerAnimations;
    std::array<xy::EmitterSettings, ParticleID::Count> m_particleEmitters;

    MapLoader m_mapLoader;
    InputParser m_playerInput;

    xy::AudioResource m_audioResource;
    xy::AudioScape m_ambience;
    xy::AudioScape m_effects;

    void initScene();
    void loadResources();
    void buildWorld();
    void loadCollision();
    void loadEnemies();
    void loadProps();
    void buildUI();

    void spawnCrate(sf::Vector2f);

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};
