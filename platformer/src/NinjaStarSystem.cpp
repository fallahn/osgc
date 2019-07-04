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

#include "NinjaStarSystem.hpp"
#include "MessageIDs.hpp"
#include "Collision.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

NinjaStarSystem::NinjaStarSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(NinjaStarSystem))
{
    requireComponent<NinjaStar>();
    requireComponent<xy::Transform>();
}

//public
void NinjaStarSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& star = entity.getComponent<NinjaStar>();
        auto& tx = entity.getComponent<xy::Transform>();
        tx.move(star.velocity * dt);

        //check surrounding area for collision
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        auto scale = tx.getScale() / 2.f;
        bounds.left = tx.getPosition().x;
        bounds.top = tx.getPosition().y;
        bounds.left -= tx.getOrigin().x * scale.x;
        bounds.top -= tx.getOrigin().y * scale.y;
        bounds.width *= scale.x;
        bounds.height *= scale.y;

        auto query = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds, CollisionGroup::StarFlags);
        for (auto other : query)
        {
            if (other != entity)
            {
                auto otherBounds = other.getComponent<xy::Transform>().getTransform().transformRect(other.getComponent<xy::BroadphaseComponent>().getArea());
                if (otherBounds.intersects(bounds))
                {
                    getScene()->destroyEntity(entity);
                    
                    auto* msg = postMessage<StarEvent>(MessageID::StarMessage);
                    msg->type = StarEvent::HitItem;
                    msg->position = tx.getPosition();
                    msg->entityHit = other;
                    msg->collisionShape = other.getComponent<xy::BroadphaseComponent>().getFilterFlags();

                    break;
                }
            }
        }
    }
}