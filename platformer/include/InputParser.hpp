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

#include <xyginext/ecs/Entity.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

struct InputBinding;

class InputParser final
{
public:
    InputParser(const InputBinding&);

    void handleEvent(const sf::Event&);
    void update();
    void setPlayerEntity(xy::Entity e) { m_playerEntity = e; }
    xy::Entity getPlayerEntity() const { return m_playerEntity; }

private:
    const InputBinding& m_inputBinding;
    xy::Entity m_playerEntity;
    float m_runningMultiplier;
    std::uint16_t m_currentInput;
};