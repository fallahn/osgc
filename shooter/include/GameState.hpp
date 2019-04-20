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

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

#include <SFML/Graphics/Shader.hpp>

struct SharedData;
class GameState final : public xy::State
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

    xy::Entity m_sideCamera;
    xy::Entity m_topCamera;

    xy::ResourceHolder m_resources;
    SpriteArray m_sprites;

    MapLoader m_mapLoader;

    sf::Shader m_cloudShader;

    void initScene();
    void loadAssets();
    void loadWorld();

    void recalcViews();
};