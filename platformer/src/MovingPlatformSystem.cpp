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

#include "MovingPlatform.hpp"

#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/util/Vector.hpp>

MovingPlatformSystem::MovingPlatformSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(MovingPlatformSystem))
{
    requireComponent<MovingPlatform>();
    requireComponent<xy::Transform>();
}

//public
void MovingPlatformSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& plat = entity.getComponent<MovingPlatform>();
        auto& tx = entity.getComponent<xy::Transform>();

        tx.move(plat.velocity * plat.speed * dt);

        auto len2 = xy::Util::Vector::lengthSquared(plat.path[plat.currentTarget] - tx.getPosition());
        if (len2 < 10) 
        {
            if (plat.forwards)
            {
                plat.currentTarget = (plat.currentTarget + 1) % plat.path.size();
                if (plat.currentTarget == plat.path.size() - 1)
                {
                    plat.forwards = false;
                }
            }
            else
            {
                plat.currentTarget = (plat.currentTarget + (plat.path.size() - 1)) % plat.path.size();
                if (plat.currentTarget == 0)
                {
                    plat.forwards = true;
                }
            }

            plat.velocity = xy::Util::Vector::normalise(plat.path[plat.currentTarget] - tx.getPosition());
        }
    }
}

//private
void MovingPlatformSystem::onEntityAdded(xy::Entity entity)
{
    XY_ASSERT(!entity.getComponent<MovingPlatform>().path.empty(), "empty path!");

    auto& plat = entity.getComponent<MovingPlatform>();   
    plat.velocity = xy::Util::Vector::normalise(plat.path[1] - plat.path[0]);
}
