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

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Clock.hpp>

struct SharedData;
class TransitionState final : public xy::State
{
public:
    TransitionState(xy::StateStack&, xy::State::Context, SharedData&);

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float dt) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Transition; }

private:
    SharedData& m_sharedData;
    sf::Sprite m_sprite;
    sf::Clock m_transitionClock;
};