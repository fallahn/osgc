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

#include "LapDotSystem.hpp"
#include "VehicleSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>

namespace
{
    const sf::Time updateTime = sf::seconds(0.1f);
    const float updateSpeed = 10.f;
}

LapDotSystem::LapDotSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(LapDotSystem))
{
    requireComponent<LapDot>();
    requireComponent<xy::Transform>();
}

//public
void LapDotSystem::process(float dt)
{
    auto& entities = getEntities();

    bool newTarget = m_updateClock.getElapsedTime() > updateTime;

    for(auto entity : entities)
    {
        auto& lapDot = entity.getComponent<LapDot>();
        auto& tx = entity.getComponent<xy::Transform>();
        const auto& vehicle = lapDot.parent.getComponent<Vehicle>();

        if (newTarget)
        {
            XY_ASSERT(lapDot.trackLength > 0, "Track length not set!");
            lapDot.target = (vehicle.totalDistance / lapDot.trackLength) * xy::DefaultSceneSize.x;
        }

        auto diff = lapDot.target - tx.getPosition().x;
        
        if (diff < -(xy::DefaultSceneSize.x / 2.f))
        {
            auto pos = tx.getPosition();
            pos.x += diff;
            tx.setPosition(pos);
        }
        else
        {
            tx.move(diff * dt * updateSpeed, 0.f);
        }
    }

    if (newTarget)
    {
        m_updateClock.restart();
    }
}