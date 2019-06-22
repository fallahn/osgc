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

#include "VehicleSystem.hpp"

#include <xyginext/ecs/System.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/util/Vector.hpp>

struct EngineAudio final
{
    static const float constexpr MaxVel = 250000.f;
};

class EngineAudioSystem final : public xy::System 
{
public:
    explicit EngineAudioSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(EngineAudioSystem))
    {
        requireComponent<EngineAudio>();
        requireComponent<Vehicle>();
        requireComponent<xy::AudioEmitter>();
    }

    void process(float) override
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            const auto& vehicle = entity.getComponent<Vehicle>();
            if ((vehicle.stateFlags & ((1 << Vehicle::Normal) | (1 << Vehicle::Disabled)/* | (1 << Vehicle::Celebrating)*/)) == 0)
            {
                entity.getComponent<xy::AudioEmitter>().stop();
            }
            else
            {
                float pitch = std::min(1.f, xy::Util::Vector::lengthSquared(vehicle.velocity) / EngineAudio::MaxVel);
                entity.getComponent<xy::AudioEmitter>().setPitch(1.f + pitch);
            }
        }
    }
};