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

#include <xyginext/ecs/System.hpp>

#include <array>

struct Bubble final
{
    enum class State
    {
        Queued,   //waiting at the side
        Queuing,  //moving to gun mount
        Mounted,  //on gun mount
        Firing,   //travelling
        Suspended,//stuck to the arena
        Dying     //bursting animation
    }state = State::Queued;

    std::int32_t gridIndex = -1;

    sf::Vector2f velocity;
    static constexpr float Speed = 1000.f;
};

struct NodeSet;
class BubbleSystem final : public xy::System
{
public:
    BubbleSystem(xy::MessageBus&, NodeSet&);

    void process(float) override;

    void resetGrid() { m_grid = {}; }

private:
    NodeSet& m_nodeSet;

    std::array<xy::Entity, 64u> m_grid;

    void doCollision(xy::Entity);
    void onEntityAdded(xy::Entity) override;
};
