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
#include <vector>

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

    std::int32_t gridIndex = -1; //where we are in the tile grid
    std::int32_t colourType = -1;

    bool testCluster = false; //true if just landed and needs to test for clusters
    bool processed = false; //used during cluster testing

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

    std::array<xy::Entity, 128u> m_grid;

    void doCollision(xy::Entity);
    std::vector<std::int32_t> fetchCluster(xy::Entity, bool matchAll = false); //returns true if bubbles were removed
    void testFloating();
    std::vector<std::int32_t> getNeighbours(std::int32_t) const;
    void onEntityAdded(xy::Entity) override;
};
