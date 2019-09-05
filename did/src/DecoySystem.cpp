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

#include "DecoySystem.hpp"
#include "AnimationSystem.hpp"

#include <xyginext/ecs/Scene.hpp>

/*
Decoys attract bees and zombies and remain
active until they are destroyed or lifetime expires
*/

namespace
{
    const float DespawnTime = 0.5f;
    const float LifeTime = 15.f;
}

DecoySystem::DecoySystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(DecoySystem))
{
    requireComponent<Decoy>();
    requireComponent<AnimationModifier>();
}

void DecoySystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& decoy = entity.getComponent<Decoy>();
        switch (decoy.state)
        {
        default:break;
        case Decoy::Spawning:
            decoy.stateTime -= dt;
            if (decoy.stateTime < 0)
            {
                decoy.stateTime = LifeTime;
                decoy.state = Decoy::Active;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
            }

            break;
        case Decoy::Active:
            decoy.stateTime -= dt;
            if (decoy.stateTime < 0 || decoy.health < 0)
            {
                decoy.stateTime = DespawnTime;
                decoy.state = Decoy::Despawning;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;
            }

            //TODO check for collision and update health?

            break;
        case Decoy::Despawning:
            //allows time for animation to play
            decoy.stateTime -= dt;
            if (decoy.stateTime < 0)
            {
                getScene()->destroyEntity(entity);
            }
            break;
        }
    }
}