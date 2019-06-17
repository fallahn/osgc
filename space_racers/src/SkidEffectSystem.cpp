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

#include "SkidEffectSystem.hpp"
#include "VehicleSystem.hpp"
#include "InputBinding.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/ParticleEmitter.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    const float MinVelocity = 940.f;
    const float MinVelocitySqr = MinVelocity * MinVelocity;
}

SkidEffectSystem::SkidEffectSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(SkidEffectSystem))
{
    requireComponent<Vehicle>();
    requireComponent<SkidEffect>();
}

//public
void SkidEffectSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& vehicle = entity.getComponent<Vehicle>();
        if (vehicle.stateFlags == (1 << Vehicle::Normal))
        {
            auto len2 = xy::Util::Vector::lengthSquared(vehicle.velocity);

            if (((vehicle.history[vehicle.lastUpdatedInput].flags & (InputFlag::Left | InputFlag::Right | InputFlag::Brake)))
                && len2 > MinVelocitySqr)
            {
                entity.getComponent<xy::ParticleEmitter>().start();
            }
            else
            {
                entity.getComponent<xy::ParticleEmitter>().stop();
            }
        }
        else
        {
            entity.getComponent<xy::ParticleEmitter>().stop();
        }
    }
}