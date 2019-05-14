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

namespace xy
{
    class NetClient;
}

//one of these exists in the 'play' state for each local player
//it reads the input based on the current binding (keyb/mouse)
//applies it to the clientside player and forwards it to the server
class InputParser final
{
public:
    InputParser(const InputBinding&, xy::NetClient* = nullptr);

    void handleEvent(const sf::Event&);
    void update(float);
    void setPlayerEntity(xy::Entity e) { m_playerEntity = e; }
    xy::Entity getPlayerEntity() const { return m_playerEntity; }

private:
    const InputBinding& m_inputBinding;
    xy::NetClient* m_netClient;
    xy::Entity m_playerEntity;
    float m_analogueMultiplier;

    std::uint16_t m_currentInput;
    sf::Clock m_clientClock;
    std::int32_t m_timeAccumulator;
};