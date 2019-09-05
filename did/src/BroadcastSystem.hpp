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

#pragma once

#include "Actor.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/System.hpp>
#include <xyginext/ecs/components/Transform.hpp>

struct BroadcastComponent final
{
    std::uint8_t buns = 0;
};

class BroadcastSystem final : public xy::System
{
public:
    explicit BroadcastSystem(xy::MessageBus& mb) 
        : xy::System(mb, typeid(BroadcastSystem))
    {
        requireComponent<BroadcastComponent>();
        requireComponent<Actor>();
        requireComponent<xy::Transform>();
    }

    void process(float) override
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            auto* msg = postMessage<MiniMapEvent>(MessageID::MiniMapUpdate);
            msg->position = entity.getComponent<xy::Transform>().getPosition();
            msg->actorID = entity.getComponent<Actor>().id;
        }
    }
};