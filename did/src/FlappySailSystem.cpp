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

#include "FlappySailSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/util/Wavetable.hpp>
#include <xyginext/util/Random.hpp>

namespace
{
    const float MaxWind = 20.f;
}

FlappySailSystem::FlappySailSystem(xy::MessageBus& mb)
    : xy::System        (mb, typeid(FlappySailSystem)),
    m_windStrength      (0.f),
    m_targetWindStrength(0.f)
{
    requireComponent<xy::Drawable>();
    requireComponent<FlappySail>();

    m_waveTable = xy::Util::Wavetable::sine(0.5f, 4.f);
}

//public
void FlappySailSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& sail = entity.getComponent<FlappySail>();
        sail.indexA = (sail.indexA + 1) % m_waveTable.size();
        sail.indexB = (sail.indexB + 1) % m_waveTable.size();

        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& vertices = drawable.getVertices();

        float offsetA = (m_windStrength + m_waveTable[sail.indexA]) + ((m_windStrength / MaxWind) * m_waveTable[sail.indexA]);
        float offsetB = (m_windStrength + m_waveTable[sail.indexB]) + ((m_windStrength / MaxWind) * m_waveTable[sail.indexB]);

        vertices[0] = { sf::Vector2f(24.f, 60.f), sf::Vector2f(sail.clothRect.left, sail.clothRect.top) };
        vertices[1] = { sf::Vector2f(70.f, 51.f), sf::Vector2f(sail.clothRect.left + sail.clothRect.width, sail.clothRect.top) };
        vertices[2] = { sf::Vector2f(70.f + offsetA, 0.f + offsetA), sf::Vector2f(sail.clothRect.left + sail.clothRect.width, sail.clothRect.top + sail.clothRect.height) };
        vertices[3] = { sf::Vector2f(24.f + offsetB, 9.f + offsetB), sf::Vector2f(sail.clothRect.left, sail.clothRect.top + sail.clothRect.height) };
        vertices[4] = { sf::Vector2f(0.f, 64.f), sf::Vector2f(sail.poleRect.left, sail.poleRect.top) };
        vertices[5] = { sf::Vector2f(74.f, 64.f), sf::Vector2f(sail.poleRect.left + sail.poleRect.width, sail.poleRect.top) };
        vertices[6] = { sf::Vector2f(74.f, 0.f), sf::Vector2f(sail.poleRect.left + sail.poleRect.width, sail.poleRect.top + sail.poleRect.height) };
        vertices[7] = { sf::Vector2f(), sf::Vector2f(sail.poleRect.left, sail.poleRect.top + sail.poleRect.height) };


        drawable.updateLocalBounds();
    }

    //smooth out wind changes a bit
    if (m_targetWindStrength > m_windStrength)
    {
        m_windStrength += (dt * (MaxWind / 4));
    }
    else
    {
        m_windStrength -= (dt * (MaxWind / 4));
    }
}

void FlappySailSystem::onEntityAdded(xy::Entity entity)
{
    auto& sail = entity.getComponent<FlappySail>();
    sail.indexA = xy::Util::Random::value(0u, m_waveTable.size() - 1);
    sail.indexB = (sail.indexA + xy::Util::Random::value(12, 32)) % m_waveTable.size();

    entity.getComponent<xy::Drawable>().getVertices().resize(8);
    entity.getComponent<xy::Drawable>().setPrimitiveType(sf::Quads);
}

void FlappySailSystem::setWindStrength(float strength)
{
    m_targetWindStrength = std::min(strength, MaxWind);
}
