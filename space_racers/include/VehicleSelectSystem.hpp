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

#include <xyginext/ecs/System.hpp>

#include <SFML/Graphics/Rect.hpp>

struct VehicleSelect final
{
    std::size_t index = 0;
    float rotation = 0.f;
};

class VehicleSelectSystem final : public xy::System
{
public:
    explicit VehicleSelectSystem(xy::MessageBus&);

    void process(float) override;

private:

    void onEntityAdded(xy::Entity) override;
};