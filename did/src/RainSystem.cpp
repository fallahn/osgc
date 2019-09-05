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

#include "RainSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/Scene.hpp>

namespace
{
    const float RainCount = 3.f;
    const float MaxDistance = Rain::Spacing * RainCount;
    const float Tolerance = 8.f;
    const float RainSpeed = 520.f;
}

RainSystem::RainSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(RainSystem))
{
    requireComponent<Rain>();
    requireComponent<xy::Transform>();
    requireComponent<xy::Sprite>();
}

void RainSystem::process(float dt)
{
    auto cameraPos = getScene()->getActiveCamera().getComponent<xy::Transform>().getPosition();

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto rainPos = tx.getPosition();
        
        //if rain is in the distance move behind the camera
        auto diff = (cameraPos.y - rainPos.y);
        if (diff > (MaxDistance - Tolerance))
        {
            tx.move(0.f, MaxDistance);
        }

        else if (diff < 0)
        {
            tx.move(0.f, -MaxDistance);
        }

        auto& spr = entity.getComponent<xy::Sprite>();
        auto rect = spr.getTextureRect();
        rect.top -= RainSpeed * dt;
        spr.setTextureRect(rect);
    }
}