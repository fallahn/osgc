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

#include "CarriableSystem.hpp"

#include <xyginext/ecs/System.hpp>
#include <xyginext/ecs/Scene.hpp>

struct TimedCarriable final
{
    float currentTime = 0.f;
    static constexpr float Timeout = 15.f;
};

class TimedCarriableSystem final : public xy::System
{
public:
    explicit TimedCarriableSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(TimedCarriableSystem))
    {
        requireComponent<TimedCarriable>();
        requireComponent<Carriable>();
    }

    void process(float dt) override
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            if (!entity.destroyed())
            {
                const auto& carriable = entity.getComponent<Carriable>();
                auto& timedCarriable = entity.getComponent<TimedCarriable>();

                if (carriable.carried)
                {
                    timedCarriable.currentTime = 0.f;
                }
                else
                {
                    timedCarriable.currentTime += dt;
                    if (timedCarriable.currentTime > TimedCarriable::Timeout)
                    {
                        getScene()->destroyEntity(entity);
                    }
                }
            }
        }
    }

private:
};
