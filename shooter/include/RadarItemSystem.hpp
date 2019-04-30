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

#include "GameConsts.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/System.hpp>
#include <xyginext/ecs/components/Transform.hpp>

struct RadarItem final
{
    xy::Entity parent;
};

class RadarItemSystem final : public xy::System
{
public:
    explicit RadarItemSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(RadarItemSystem))
    {
        requireComponent<xy::Transform>();
        requireComponent<RadarItem>();
    }

    void process(float) override
    {
        auto& entities = getEntities();
        for (auto& entity : entities)
        {
            auto& item = entity.getComponent<RadarItem>();

            if (item.parent.destroyed())
            {
                getScene()->destroyEntity(entity);
            }
            else
            {
                auto otherPos = item.parent.getComponent<xy::Transform>().getPosition();
                auto& tx = entity.getComponent<xy::Transform>();
                tx.setPosition(ConstVal::BackgroundPosition.x + (otherPos.y / 2.f), ConstVal::MaxDroneHeight);
            }
        }
    }
};
