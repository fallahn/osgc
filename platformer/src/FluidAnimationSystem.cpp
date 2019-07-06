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

#include "FluidAnimationSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>

FluidAnimationSystem::FluidAnimationSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(FluidAnimationSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<Fluid>();
}

//public
void FluidAnimationSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        for (auto& v : verts)
        {
            v.texCoords.x -= dt * 4.f;
        }

        auto& fluid = entity.getComponent<Fluid>();
        fluid.currentTime += dt;
        if (fluid.currentTime > fluid.frameTime)
        {
            fluid.currentTime = 0.f;

            for (auto& v : verts)
            {
                v.texCoords.y += fluid.frameHeight;
            }
        }
    }
}