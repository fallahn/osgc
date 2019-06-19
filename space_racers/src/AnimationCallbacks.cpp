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

#include "AnimationCallbacks.hpp"

#include <xyginext/util/Wavetable.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

namespace
{
    const std::size_t vertexCount = 12;
}

StartRingAnimator::StartRingAnimator()
{
    m_waveTable = xy::Util::Wavetable::sine(0.5f);
    for (auto& f : m_waveTable)
    {
        f += 1.f;
        f /= 2.f;
        f *= 255.f;
    }

    for (auto i = 0u; i < vertexCount; ++i)
    {
        m_indices.push_back((i / 4) * (m_waveTable.size() / 4));
    }

    //auto offset = m_waveTable.size() / 3;
    //m_indices[4] = (m_indices[4] + offset) % m_waveTable.size();
    //m_indices[5] = (m_indices[5] + offset) % m_waveTable.size();
    //m_indices[6] = (m_indices[6] + offset) % m_waveTable.size();
    //m_indices[7] = (m_indices[7] + offset) % m_waveTable.size();

    //offset *= 2;
    //m_indices[8] = (m_indices[5] + offset) % m_waveTable.size();
    //m_indices[9] = (m_indices[6] + offset) % m_waveTable.size();
    //m_indices[10] = (m_indices[10] + offset) % m_waveTable.size();
    //m_indices[11] = (m_indices[11] + offset) % m_waveTable.size();
}

//public
void StartRingAnimator::operator()(xy::Entity entity, float)
{
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    XY_ASSERT(verts.size() == vertexCount, "");
    
    for (auto i = 0u; i < vertexCount; ++i)
    {
        verts[i].color.a = static_cast<sf::Uint8>(m_waveTable[m_indices[i]]);
        m_indices[i] = (m_indices[i] + 1) % m_waveTable.size();
    }
}