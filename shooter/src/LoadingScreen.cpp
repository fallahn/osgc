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

    const char chuff[] = "buns flaps dicketry makes speefcake libblelow hibberny. Woops barnacles cleft#";
    std::size_t chuffIdx = 0;
}

LoadingScreen::LoadingScreen()
    : m_currentFrameTime  (0.f)
{
    m_imageData =
    {
        255, 128, 0, 255,
        0, 128, 255, 255,
        128, 255, 0, 255,
        0, 255, 128, 255,
        255, 128, 128, 255,
        128, 128, 255, 255,
        0, 128, 255, 255,
        128, 255, 128, 255,
        255, 0, 128, 255,
        128, 0, 128, 255,
        255, 128, 0, 255,
        0, 128, 255, 255,
        128, 255, 0, 255,
        0, 255, 128, 255,
        255, 255, 128, 255,
        128, 255, 0, 255,
        255, 128, 128, 255,
        128, 255, 128, 255,
        255, 0, 128, 255,
        128, 0, 128, 255,
        255, 128, 0, 255,
        0, 128, 255, 255,
        255, 255, 0, 255,
        0, 255, 255, 255,
        255, 128, 128, 255,
        128, 128, 255, 255,
        255, 128, 0, 255,
        128, 255, 128, 255,
        255, 0, 128, 255,
        128, 0, 128, 255,
        0, 128, 255, 255,
        255, 0, 128, 255,
        128, 0, 255, 255,
    };

    m_texture.create(1, 33);
    m_texture.update(m_imageData.data());

    m_sprite.setTexture(m_texture, true);
    m_sprite.setScale(xy::DefaultSceneSize.x, xy::DefaultSceneSize.y / 32.f);

    m_vertices =
    {
        sf::Vertex(sf::Vector2f(262.f, 94.f), sf::Color::Black),
        {sf::Vector2f(xy::DefaultSceneSize.x - 262.f, 94.f), sf::Color::Black},
        {sf::Vector2f(xy::DefaultSceneSize.x - 262.f, xy::DefaultSceneSize.y - 94.f), sf::Color::Black},
        {sf::Vector2f(262.f, xy::DefaultSceneSize.y - 94.f), sf::Color::Black}
    };
}

//public
void LoadingScreen::update(float dt)
{
    static int offset = 0;

    m_currentFrameTime += dt;
    if (m_currentFrameTime > frametime)
    {
        m_currentFrameTime = 0.f;

        offset++;
        if (offset % 2 == 0)
        {
            m_sprite.move(0.f, 32.f);
        }
        else
        {
            m_sprite.move(0.f, -32.f);
        }

        for (auto i = 0u; i <  m_imageData.size(); ++i)
        {
            if (i % 4 == 1)
            {
                m_imageData[i] ^= chuff[chuffIdx];
                chuffIdx = (chuffIdx + 1) % 79;
            }
        }
        
        m_texture.update(m_imageData.data());
    }
}

//private
void LoadingScreen::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    rt.draw(m_sprite, states);
    rt.draw(m_vertices.data(), m_vertices.size(), sf::Quads);
}