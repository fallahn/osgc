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

#include "ActorSystem.hpp"
#include "Actor.hpp"

#include <xyginext/ecs/components/Transform.hpp>

ActorSystem::ActorSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ActorSystem)),
    m_aliveCount(0)
{
    requireComponent<Actor>();
}

void ActorSystem::process(float)
{
    m_aliveCount = 0;
    for (const auto& entity : getEntities())
    {
        if (entity.getComponent<Actor>().counted)
        {
            m_aliveCount++;
        }
    }
}