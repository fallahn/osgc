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

#include "SeagullSystem.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const sf::FloatRect innerBounds(64.f, 64.f, Global::IslandSize.x - 128.f, Global::IslandSize.y - 128.f);
    const float borderWidth = 128.f;
}

SeagullSystem::SeagullSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(SeagullSystem))
{
    requireComponent<Seagull>();
    requireComponent<xy::Transform>();
    requireComponent<xy::AudioEmitter>();
}

//public
void SeagullSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& seagull = entity.getComponent<Seagull>();

        //update call time and play sound if needed
        seagull.currentTime += dt;
        if (seagull.currentTime > seagull.callTime)
        {
            seagull.currentTime = 0.f;
            if (seagull.enabled)
            {
                entity.getComponent<xy::AudioEmitter>().play();
            }
        }

        //if moving update movement and keep within bounds
        if (seagull.moves)
        {
            //move within large bounds
            auto& tx = entity.getComponent<xy::Transform>();
            tx.move(seagull.velocity * dt);

            auto pos = tx.getPosition();
            if (pos.x < -borderWidth)
            {
                seagull.velocity.x = -seagull.velocity.x;
                tx.setPosition(-borderWidth, pos.y);
            }
            else if (pos.x > (Global::IslandSize.x + borderWidth))
            {
                seagull.velocity.x = -seagull.velocity.x;
                tx.setPosition(Global::IslandSize.x + borderWidth, pos.y);
            }

            if (pos.y < -borderWidth)
            {
                seagull.velocity.y = -seagull.velocity.y;
                tx.setPosition(pos.x, -borderWidth);
            }
            else if (pos.y > (Global::IslandSize.y + borderWidth))
            {
                seagull.velocity.y = -seagull.velocity.y;
                tx.setPosition(pos.x, Global::IslandSize.y + borderWidth);
            }

            //mute if in small bounds
            if (innerBounds.contains(pos))
            {
                entity.getComponent<xy::AudioEmitter>().setVolume(0.f);
            }
            else
            {
                entity.getComponent<xy::AudioEmitter>().setVolume(1.f);
            }
        }
    }
}