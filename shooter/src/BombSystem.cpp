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

#include "Bomb.hpp"
#include "MessageIDs.hpp"
#include "GameConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

namespace
{
    const float Gravity = 9.9f;
    const float MaxBombHeight = 300.f;
}

BombSystem::BombSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(BombSystem))
{
    requireComponent<Bomb>();
    requireComponent<xy::Transform>();
}

void BombSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& bomb = entity.getComponent<Bomb>();
        if (bomb.type == Bomb::Side)
        {
            updateSide(entity, dt);
        }
        else if (bomb.type == Bomb::Top)
        {
            updateTop(entity, dt);
        }
    }
}

//private
void BombSystem::updateSide(xy::Entity entity, float dt)
{
    auto& bomb = entity.getComponent<Bomb>();
    auto& tx = entity.getComponent<xy::Transform>();

    //update the position/velocity as if viewed from the top
    bomb.position += bomb.velocity * dt;
    bomb.velocity *= 0.9f;

    //update the gravity and use it to place the entity on screen
    bomb.gravity += Gravity;
    
    auto pos = tx.getPosition();
    pos.x = ConstVal::BackgroundPosition.x + (bomb.position.y / 2.f);
    pos.y += bomb.gravity * dt;
    tx.setPosition(pos);

    //check Y pos - if more than ground position then destroy ent and
    //raise a message with position data to create an explosion
    if (tx.getPosition().y > MaxBombHeight)
    {
        auto* msg = postMessage<BombEvent>(MessageID::BombMessage);
        msg->type = BombEvent::Exploded;
        msg->position = bomb.position;

        getScene()->destroyEntity(entity);
    }
}

void BombSystem::updateTop(xy::Entity entity, float dt)
{
    //this is an approx visualisation    
    auto& bomb = entity.getComponent<Bomb>();
    auto& tx = entity.getComponent<xy::Transform>();

    //apply the same calculations as side view but use it to draw the
    //top down position
    bomb.position += bomb.velocity * dt;
    bomb.velocity *= 0.9f;
    tx.setPosition(bomb.position);

    //add some scaling for fake perspective
    tx.scale(0.95f, 0.95f);

    //destroy after gravity calc says we ought to be at some height?
    bomb.gravity += Gravity;
    bomb.height += bomb.gravity * dt;

    if (bomb.height > MaxBombHeight)
    {
        getScene()->destroyEntity(entity);
    }
}