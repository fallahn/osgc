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

#include <vector>
#include <string>

struct SharedData;
class IntroState final : public xy::State
{
public:
    IntroState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Intro; }

private:
    xy::Scene m_scene;
    SharedData& m_sharedData;
    xy::ResourceHandler m_resources;

    std::vector<std::string> m_lines;
    std::size_t m_currentLine;

    struct Action final
    {
        std::function<void(float)> update;
        std::function<void()> finish;
        bool finished = false;
    };
    std::vector<Action> m_actions;
    std::vector<float> m_actionTimes;
    std::size_t m_currentAction;

    void build();
    void nextLine();
};
