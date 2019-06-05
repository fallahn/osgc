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

#include "AIDriverSystem.hpp"
#include "VehicleSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>

AIDriverSystem::AIDriverSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(AIDriverSystem))
{
    requireComponent<AIDriver>();
    requireComponent<Vehicle>();
}

//public
void AIDriverSystem::process(float dt)
{
    auto frameTime = static_cast<std::int32_t>(dt * 1000000.f);

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& ai = entity.getComponent<AIDriver>();
        ai.timestamp += frameTime;

        //get target

        //get forward vector
        sf::Transform tx; //TODO use our LUT transform
        tx.rotate(entity.getComponent<xy::Transform>().getRotation());
        sf::Vector2f forwardVec = tx.transformPoint({ 1.f, 0.f });

        //adjust steering based on which side of forward ray target is on

        //adjust acceleration multiplier based on distance to target (ie slow down when nearer)

        //TODO sweep for collidable objects and steer away from? solids or space
        //TODO faux acceleration by modifying input? Could use this for differing types of AI

        //update timestamp and apply input to vehicle
    }
}