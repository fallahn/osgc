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

#include "MapLoader.hpp"
#include "ResourceIDs.hpp"
#include "InputParser.hpp"
#include "AnimationMap.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/core/ConsoleClient.hpp>
#include <xyginext/gui/GuiClient.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/ParticleEmitter.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

struct SharedData;
class MenuState final : public xy::State, public xy::ConsoleClient, public xy::GuiClient
{
public:
    MenuState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:
    SharedData& m_sharedData;
    xy::Scene m_backgroundScene;

    xy::ShaderResource m_shaders;
    xy::ResourceHandler m_resources;

    std::array<std::size_t, TextureID::Menu::Count> m_textureIDs;
    SpriteArray<SpriteID::GearBoy::Count> m_sprites;
    AnimationMap<AnimID::Player::Count> m_playerAnimations;
    std::size_t m_fontID;
    std::array<xy::EmitterSettings, ParticleID::Count> m_particleEmitters;

    MapLoader m_mapLoader;
    InputParser m_playerInput;

    void initScene();
    void loadResources();
    void buildBackground();
    void buildMenu();

    void spawnCrate(sf::Vector2f);

    void updateLoadingScreen(float, sf::RenderWindow&) override;
};