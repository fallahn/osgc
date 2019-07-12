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

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

#include <SFML/Graphics/Shader.hpp>
#include <SFML/System/Clock.hpp>

struct SharedData;
class SummaryState final : public xy::State
{
public:
    SummaryState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Summary; }

private:
    xy::Scene m_scene;
    SharedData& m_sharedData;
    xy::ResourceHandler m_resources;

    sf::Shader m_shader;

    enum
    {
        AddCoins, AddTime, Completed
    }m_state;
    sf::Clock m_stateClock;

    enum
    {
        Score, Coins, Time, OK, Count
    };
    std::array<xy::Entity, Count> m_textEnts;

    void build();

    void addCoins();
    void addTime();
};
