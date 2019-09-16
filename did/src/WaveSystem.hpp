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
#include <xyginext/ecs/components/Sprite.hpp>


struct Wave final
{
    static constexpr float MinScale = 1.9f;
    static constexpr float MaxScale = 2.2f;
    float currentScale = MinScale;
};

class WaveSystem final : public xy::System
{
public:
    explicit WaveSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(WaveSystem))
    {
        requireComponent<xy::Transform>();
        requireComponent<xy::Sprite>();
        requireComponent<Wave>();
    }

    void process(float dt) override
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            auto& wave = entity.getComponent<Wave>();
            auto& tx = entity.getComponent<xy::Transform>();
            auto& sprite = entity.getComponent<xy::Sprite>();

            float transparency = 1.f - ((wave.currentScale - Wave::MinScale) / (Wave::MaxScale - Wave::MinScale));
            transparency = std::min(std::max(transparency, 0.f), 1.f);

            sf::Color c(255, 255, 255, static_cast<sf::Uint8>(255.f * transparency));
            sprite.setColour(c);

            wave.currentScale += (dt / 1.5f) * transparency;
            tx.setScale(wave.currentScale, wave.currentScale);

            if (transparency < 0.05)
            {
                wave.currentScale = Wave::MinScale;
            }
        }
    }

private:
};