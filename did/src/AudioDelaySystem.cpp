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

#include "AudioDelaySystem.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    const float MinDistance = xy::Util::Vector::lengthSquared(Global::IslandSize / 4.f);
}

AudioDelaySystem::AudioDelaySystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(AudioDelaySystem))
{
    requireComponent<AudioDelay>();
    requireComponent<xy::Transform>();
    requireComponent<xy::AudioEmitter>();
}

//public
void AudioDelaySystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& delay = entity.getComponent<AudioDelay>();

        if (delay.active)
        {
            auto& emitter = entity.getComponent<xy::AudioEmitter>();
            if (emitter.getStatus() == xy::AudioEmitter::Stopped)
            {
                delay.active = false;
            }
            else
            {
                auto currPos = getScene()->getActiveListener().getComponent<xy::Transform>().getWorldPosition();
                auto currDist = xy::Util::Vector::lengthSquared(currPos - entity.getComponent<xy::Transform>().getPosition());
                currDist = std::max(0.f, currDist - MinDistance);
                
                //TODO use min distance to set the ratio to 0
                float ratio = std::min(1.f, std::sqrt(currDist) / std::sqrt(delay.startDistance));
                emitter.setVolume(delay.startVolume * ratio);
            }
        }
    }
}