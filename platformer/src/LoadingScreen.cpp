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

#include "LoadingScreen.hpp"

#include <xyginext/core/FileSystem.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include <array>

namespace
{
    constexpr float frametime = 1.f / 50.f;
    const float scale = 4.f;
    const float tailSize = 16.f;
}

LoadingScreen::LoadingScreen()
    : m_currentFrameTime  (0.f),
    m_tailCount(0)
{
    m_texture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/loading.png");

    m_frameSize = sf::Vector2f(m_texture.getSize());
    m_frameSize.y /= 2.f;
    m_frameSize.x -= 16.f;

    m_vertices =
    {
        sf::Vertex(),
        {sf::Vector2f(m_frameSize.x, 0.f), sf::Vector2f(m_frameSize.x, 0.f)},
        {m_frameSize, m_frameSize},
        {sf::Vector2f(0.f, m_frameSize.y), sf::Vector2f(0.f, m_frameSize.y)}
    };

    m_transform.translate(40.f + (MaxTail * (tailSize * scale)), 40.f);
    m_transform.scale(scale, scale);
}

//public
void LoadingScreen::update(float dt)
{
    m_currentFrameTime += dt;
    if (m_currentFrameTime > frametime)
    {
        m_currentFrameTime = 0.f;

        static int currFrame = 0;
        currFrame = (currFrame + 1) % 2;

        m_vertices[0].texCoords.y = currFrame * m_frameSize.y;
        m_vertices[1].texCoords.y = currFrame * m_frameSize.y;
        m_vertices[2].texCoords.y = (currFrame * m_frameSize.y) + m_frameSize.y;
        m_vertices[3].texCoords.y = (currFrame * m_frameSize.y) + m_frameSize.y;

        if (m_tailCount < MaxTail)
        {
            m_vertices.emplace_back(sf::Vector2f(m_frameSize.x + (tailSize * m_tailCount), 0.f), sf::Vector2f(m_frameSize.x, 0.f));
            m_vertices.emplace_back(sf::Vector2f((m_frameSize.x + (tailSize * m_tailCount)) + tailSize, 0.f), sf::Vector2f(m_frameSize.x + tailSize, 0.f));
            m_vertices.emplace_back(sf::Vector2f((m_frameSize.x + (tailSize * m_tailCount)) + tailSize, m_frameSize.y), sf::Vector2f(m_frameSize.x + tailSize, m_frameSize.y));
            m_vertices.emplace_back(sf::Vector2f(m_frameSize.x + (tailSize * m_tailCount), m_frameSize.y), sf::Vector2f(m_frameSize.x, m_frameSize.y));

            m_transform.translate(-tailSize, 0.f);

            m_tailCount++;
        }
    }
}

//private
void LoadingScreen::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    states.texture = &m_texture;
    states.transform = m_transform;
    rt.draw(m_vertices.data(), m_vertices.size(), sf::Quads, states);
}