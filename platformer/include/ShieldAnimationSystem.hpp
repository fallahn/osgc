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

struct ShieldAnim final
{
    std::size_t indexX = 0;
    std::size_t indexY = 0;

    static constexpr float Width = 32.f;
    static constexpr float Height = 8.f;
};

class ShieldAnimationSystem final : public xy::System
{
public:
    explicit ShieldAnimationSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(ShieldAnimationSystem))
    {
        requireComponent<xy::Transform>();
        requireComponent<ShieldAnim>();

        m_wavetable = xy::Util::Wavetable::sine(1.f);
    }

    void process(float) override
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            auto& shield = entity.getComponent<ShieldAnim>();
            auto& tx = entity.getComponent<xy::Transform>();

            tx.setPosition((m_wavetable[shield.indexX] * ShieldAnim::Width), m_wavetable[shield.indexY] * ShieldAnim::Height);

            shield.indexX = (shield.indexX + 1) % m_wavetable.size();
            shield.indexY = (shield.indexY + 1) % m_wavetable.size();
        }
    }

private:
    std::vector<float> m_wavetable;

    void onEntityAdded(xy::Entity entity) override
    {
        entity.getComponent<ShieldAnim>().indexY = m_wavetable.size() / 4;
    }
};
