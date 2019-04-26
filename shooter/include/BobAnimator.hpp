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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/System.hpp>
#include <xyginext/util/Wavetable.hpp>

#include <vector>

struct Bob final
{
    std::size_t index = 0;
};

class BobSystem final : public xy::System
{
public:
    explicit BobSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(BobSystem))
    {
        m_waveTable = xy::Util::Wavetable::sine(0.8f, 0.25f);
        requireComponent<Bob>();
        requireComponent<xy::Transform>();
    }

    void process(float) override
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            auto& bob = entity.getComponent<Bob>();
            auto& tx = entity.getComponent<xy::Transform>();

            tx.move(0.f, m_waveTable[bob.index]);
            bob.index = (bob.index + 1) % m_waveTable.size();
        }
    }

private:
    std::vector<float> m_waveTable;
};