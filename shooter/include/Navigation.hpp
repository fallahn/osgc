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

#include <xyginext/ecs/Entity.hpp>

#include <array>
#include <cstdint>

struct Node final
{
    std::uint32_t ID = 0u;
    static constexpr std::array<float, 4u> Bounds = { -12.f, -12.f, 24.f, 24.f }; //supposed to be FloatRect but idk wtf
};

struct Navigator final
{
    xy::Entity target;
    std::uint32_t previousNode;
};