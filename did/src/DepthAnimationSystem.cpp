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

#include "DepthAnimationSystem.hpp"
#include "Sprite3D.hpp"
#include "CollisionBounds.hpp"
#include "ClientWeaponSystem.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/core/App.hpp>

DepthAnimationSystem::DepthAnimationSystem(xy::MessageBus& mb)
    :xy::System(mb, typeid(DepthAnimationSystem))
{
    requireComponent<DepthComponent>();
    requireComponent<CollisionComponent>();
    requireComponent<Sprite3D>();
}

//public 
void DepthAnimationSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& collision = entity.getComponent<CollisionComponent>();
        const float height = entity.getComponent<xy::Sprite>().getTextureBounds().height * 0.66f; //bit of a kludge as actual sprite doesn't draw right to top of the bounds
        const float offset = height * collision.water;
        entity.getComponent<Sprite3D>().verticalOffset = offset;

        auto& depthComponent = entity.getComponent<DepthComponent>();
        XY_ASSERT(depthComponent.ringEffect.hasComponent<xy::Transform>(), "Invalid entity!");
        //XY_ASSERT(depthComponent.playerWeapon.hasComponent<Sprite3D>(), "Invalid entity!");

        if (depthComponent.playerWeapon.isValid()) //barrels have no weapon
        {
            depthComponent.playerWeapon.getComponent<Sprite3D>().verticalOffset =
                depthComponent.playerWeapon.getComponent<ClientWeapon>().spriteOffset + offset;
        }

        if (collision.water > 0)
        {
            depthComponent.ringEffect.getComponent<xy::Transform>().setScale(1.f, 1.f);
        }
        else
        {
            depthComponent.ringEffect.getComponent<xy::Transform>().setScale(0.f, 0.f);
        }
    }
}