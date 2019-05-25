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

#include "NetActor.hpp"
#include "VehicleSystem.hpp"
#include "AsteroidSystem.hpp"
#include "ActorIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>

NetActorSystem::NetActorSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(NetActorSystem))
{
    requireComponent<NetActor>();
    requireComponent<xy::Transform>();
}

//public
void NetActorSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& actor = entity.getComponent<NetActor>();
        if (actor.actorID == ActorID::Roid)
        {
            actor.velocity = entity.getComponent<Asteroid>().getVelocity();
        }
        else
        {
            actor.velocity = entity.getComponent<Vehicle>().velocity;
        }
    }
}