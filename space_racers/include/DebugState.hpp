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
#include "InputParser.hpp"
#include "MapParser.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/gui/GuiClient.hpp>

#include <vector>

class DebugState final : public xy::State, public xy::GuiClient
{
public:
    DebugState(xy::StateStack&, xy::State::Context, SharedData&);
    ~DebugState();

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Debug; }

private:
    SharedData& m_sharedData;
    xy::Scene m_gameScene;
    //sf::RenderTexture m_gameBuffer;
    //sf::RenderWindow m_lightingBuffer;
    //xy::Scene m_outputScene;
    //xy::Scene m_uiScene;

    xy::ResourceHolder m_resources;

    InputParser m_playerInput;
    InputParser m_otherInput;

    MapParser m_mapParser;

    void initScene();
    void loadResources();
    void buildWorld();
    void addLocalPlayers();
};