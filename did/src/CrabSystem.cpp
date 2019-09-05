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

#include "CrabSystem.hpp"
#include "Actor.hpp"
#include "AnimationSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>

namespace
{
    const float MoveSpeed = 20.f;
}

CrabSystem::CrabSystem(xy::MessageBus& mb)
    :xy::System(mb, typeid(CrabSystem))
{
    requireComponent<Crab>();
    requireComponent<Actor>();
    requireComponent<xy::Transform>();
    requireComponent<AnimationModifier>();
}

//public
void CrabSystem::handleMessage(const xy::Message&)
{

}

void CrabSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& crab = entity.getComponent<Crab>();
        auto& tx = entity.getComponent<xy::Transform>();
        switch (crab.state)
        {
        default: break;
        case Crab::State::Digging:
            crab.thinkTime -= dt;
            if (crab.thinkTime < 0)
            {
                getScene()->destroyEntity(entity);
            }
            break;
        case Crab::State::Running:
            tx.move(-MoveSpeed * dt, 0.f);
            if (tx.getPosition().x < (crab.spawnPosition.x - crab.maxTravel))
            {
                crab.state = Crab::State::Digging;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;
            }
            break;
        }
    }
}

