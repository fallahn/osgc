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

#include "LightningSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/util/Random.hpp>

LightningSystem::LightningSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(LightningSystem)),
    m_offsetIndex(0)
{
    requireComponent<Lightning>();
    requireComponent<xy::Drawable>();

    for (auto i = 0; i < 50; ++i)
    {
        m_offsets.emplace_back(xy::Util::Random::value(-5.f, 5.f), xy::Util::Random::value(-5.f, 5.f));
    }
}

//public
void LightningSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& lightning = entity.getComponent<Lightning>();
        
        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();

        verts[0].position = lightning.start + m_offsets[m_offsetIndex];
        m_offsetIndex = (m_offsetIndex + 1) % m_offsets.size();

        for (auto i = 1u; i <= lightning.points.size(); ++i)
        {
            verts[i].position = lightning.points[i - 1] + m_offsets[m_offsetIndex];
            m_offsetIndex = (m_offsetIndex + 1) % m_offsets.size();
        }
        verts[9].position = lightning.end + m_offsets[m_offsetIndex];
        m_offsetIndex = (m_offsetIndex + 1) % m_offsets.size();

        drawable.updateLocalBounds();
    }
}

//private
void LightningSystem::onEntityAdded(xy::Entity entity)
{
    auto& lightning = entity.getComponent<Lightning>();
    auto dist = lightning.end - lightning.start;
    dist /= 10.f;

    for (auto i = 1u; i <= lightning.points.size(); ++i)
    {
        lightning.points[i - 1] = lightning.start + (dist * static_cast<float>(i));
    }

    entity.getComponent<xy::Drawable>().setPrimitiveType(sf::LineStrip);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.resize(10);
    for (auto& v : verts) v.color = sf::Color::Cyan;
}