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

#include "SkullShieldSystem.hpp"
#include "CollisionBounds.hpp"
#include "AnimationSystem.hpp"
#include "QuadTreeFilter.hpp"
#include "PlayerSystem.hpp"
#include "Actor.hpp"

#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/Scene.hpp>

namespace
{
    const float LifeTime = 45.f;
    const float DespawnTime = 1.f;
}

SkullShieldSystem::SkullShieldSystem(xy::MessageBus& mb)
    :xy::System(mb, typeid(SkullShieldSystem))
{
    requireComponent<SkullShield>();
    requireComponent<CollisionComponent>();
    requireComponent<AnimationModifier>();
}

//public
void SkullShieldSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& shield = entity.getComponent<SkullShield>();
        shield.stateTime -= dt;

        switch (shield.state)
        {
        default:
        case SkullShield::Despawn:
            if (shield.stateTime < 0)
            {
                getScene()->destroyEntity(entity);
            }
            break;
        case SkullShield::Idle:
            doCollision(entity);

            if (shield.stateTime < 0)
            {
                shield.stateTime = DespawnTime;
                shield.state = SkullShield::Despawn;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;
                entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0); //turn off collision
            }
            break;
        case SkullShield::Spawn:
            //does nothing but let anim play
            if (shield.stateTime < 0)
            {
                shield.state = SkullShield::Idle;
                shield.stateTime = LifeTime;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
                entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::SkullShield);
            }
            break;
        }
    }
}


//private
void SkullShieldSystem::doCollision(xy::Entity entity)
{
    const auto& collision = entity.getComponent<CollisionComponent>();
    for (auto i = 0u; i < collision.manifoldCount; ++i)
    {
        if (collision.manifolds[i].ID == ManifoldID::Player)
        {
            if (collision.manifolds[i].otherEntity.getComponent<Actor>().id
                != entity.getComponent<SkullShield>().owner)
            {
                entity.getComponent<SkullShield>().state = SkullShield::Despawn;
                entity.getComponent<SkullShield>().stateTime = DespawnTime;
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Die;
                entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0); //turn off collision
            }
        }
    }
}