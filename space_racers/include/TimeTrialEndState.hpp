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

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <SFML/System/Clock.hpp>

class TimeTrialSummaryState final : public xy::State
{
public:
    TimeTrialSummaryState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override { return StateID::TimeTrialSummary; }

private:
    SharedData& m_sharedData;
    std::array<std::size_t, TextureID::Menu::Count> m_textureIDs;

    xy::Scene m_scene;

    sf::Clock m_delayClock;

    bool m_shown;

    void initScene();
    void buildMenu();
};