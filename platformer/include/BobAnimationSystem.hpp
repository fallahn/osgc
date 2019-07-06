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
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Wavetable.hpp>
#include <xyginext/util/Random.hpp>

struct BobAnimation final
{
    std::size_t index = 0;
    xy::Entity parent;
};

class BobAnimationSystem final : public xy::System 
{
public:
    BobAnimationSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(BobAnimationSystem))
    {
        requireComponent<BobAnimation>();
        requireComponent<xy::Transform>();

        m_waveTable = xy::Util::Wavetable::sine(2.f, 5.f);
    }

    void process(float) override
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            auto& bob = entity.getComponent<BobAnimation>();
            auto& tx = entity.getComponent<xy::Transform>();

            auto position = tx.getPosition();
            position.y = m_waveTable[bob.index];
            tx.setPosition(position);

            bob.index = (bob.index + 1) % m_waveTable.size();

            if (bob.parent.isValid() && bob.parent.destroyed())
            {
                getScene()->destroyEntity(entity);
            }
        }
    }

private:
    std::vector<float> m_waveTable;

    void onEntityAdded(xy::Entity entity) override
    {
        entity.getComponent<BobAnimation>().index = xy::Util::Random::value(0, m_waveTable.size() - 1);
    }
};