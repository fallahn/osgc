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

//for complicated reasons this is used to create a matrix containing the
//inverse rotation of a drawable so it can be used in directional lighting
//shaders. See VehicleShader.inl for more info

#pragma once

#include "Util.hpp"

#include <xyginext/ecs/System.hpp>
#include <xyginext/ecs/components/Transform.hpp>

struct InverseRotation final
{
    Transform matrix;
};

class InverseRotationSystem final : public xy::System 
{
public:
    explicit InverseRotationSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(InverseRotationSystem))
    {
        requireComponent<xy::Transform>();
        requireComponent<InverseRotation>();
    }

    void process(float)
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            entity.getComponent<InverseRotation>().matrix.setRotation(-entity.getComponent<xy::Transform>().getRotation());
        }
    }
};