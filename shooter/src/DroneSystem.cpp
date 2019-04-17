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

#include "Drone.hpp"
#include "CommandIDs.hpp"
#include "GameConsts.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

namespace
{
    const float MaxVelocity = 800.f;
    const float MaxVelocitySqr = MaxVelocity * MaxVelocity;
}

DroneSystem::DroneSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(DroneSystem))
{
    requireComponent<xy::Transform>();
    requireComponent<Drone>();
}

//public
void DroneSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& drone = entity.getComponent<Drone>();

        //clamp to max velocity
        auto len2 = xy::Util::Vector::lengthSquared(drone.velocity);
        if (len2 > MaxVelocitySqr)
        {
            drone.velocity *= (MaxVelocitySqr / len2);
        }

        auto& tx = entity.getComponent<xy::Transform>();
        auto pos = tx.getPosition();
        pos += drone.velocity * dt;
        pos.x = xy::Util::Math::clamp(pos.x, ConstVal::MapArea.left, ConstVal::MapArea.width);
        pos.y = xy::Util::Math::clamp(pos.y, ConstVal::MapArea.top, ConstVal::MapArea.height);
        tx.setPosition(pos);
        
        drone.velocity *= 0.9f;

        float sidePos = ConstVal::BackgroundPosition.x + (pos.y / 2.f);

        xy::Command cmd;
        cmd.targetFlags = CommandID::PlayerSide;
        cmd.action = [sidePos](xy::Entity e, float)
        {
            auto& tx = e.getComponent<xy::Transform>();
            auto pos = tx.getPosition();
            pos.x = sidePos;
            tx.setPosition(pos);
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}