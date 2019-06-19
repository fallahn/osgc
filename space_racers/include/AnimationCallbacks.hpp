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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Entity.hpp>

#include <vector>

class ScaleCallback final
{
public:
    void operator()(xy::Entity e, float dt)
    {
        const float scaleFactor = dt * 56.f;
        e.getComponent<xy::Transform>().scale(scaleFactor, scaleFactor);
    }
private:
};

class StartRingAnimator final
{
public:
    StartRingAnimator();

    void operator() (xy::Entity, float);

private:
    std::vector<float> m_waveTable;
    std::vector<std::size_t> m_indices;
};