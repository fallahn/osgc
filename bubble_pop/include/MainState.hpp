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

#include "ResourceIDs.hpp"
#include "LevelData.hpp"
#include "PlayerInput.hpp"
#include "NodeSet.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

#include <array>

class MainState final : public xy::State
{
public:
    MainState(xy::StateStack&, xy::State::Context);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:

    xy::Scene m_scene;
    xy::ResourceHandler m_resources;
    std::array<xy::Sprite, BubbleID::Count> m_bubbleSprites;
    std::array<xy::Sprite, SpriteID::Count> m_sprites;
    std::array<std::size_t, TextureID::Count> m_textures;

    PlayerInput m_playerInput;

    std::size_t m_currentLevel;
    std::vector<LevelData> m_levels;

    NodeSet m_nodeSet;

    void initScene();
    void loadResources();
    void loadLevelData();
    void buildArena();

    void activateLevel();
};