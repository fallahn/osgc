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
#include "MapLoader.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/gui/GuiClient.hpp>

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>

struct SharedData;
class EditorState final : public xy::State, public xy::GuiClient
{
public:
    EditorState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const { return StateID::Editor; }

private:
    SharedData& m_sharedData;
    xy::Scene m_scene;
    xy::ResourceHandler m_resources;
    xy::ShaderResource m_shaders;

    std::vector<xy::Entity> m_mapEntities;

    void initScene();
    void loadResources();
    void initUI();

    MapLoader m_mapLoader;
    std::string m_currentMap;
    void loadMap();
};
