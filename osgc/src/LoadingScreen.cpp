/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "LoadingScreen.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include <array>

namespace
{
    constexpr float frametime = 1.f / 20.f;

    std::array<sf::IntRect, 8u> frames =
    {
        sf::IntRect(0,0,36,60),
        sf::IntRect(36,0,36,60),
        sf::IntRect(72,0,36,60),
        sf::IntRect(108,0,36,60),
        sf::IntRect(144,0,36,60),
        sf::IntRect(108,0,36,60),
        sf::IntRect(72,0,36,60),
        sf::IntRect(36,0,36,60)
    };
}

LoadingScreen::LoadingScreen()
    : m_currentFrameTime(0.f),
    m_frameIndex        (0)
{
    m_texture.loadFromFile("assets/images/loading.png");
    m_sprite.setTexture(m_texture);
    m_sprite.setTextureRect(frames[0]);
    m_sprite.setPosition(40.f, 20.f);
}

//public
void LoadingScreen::update(float dt)
{
    m_currentFrameTime += dt;
    if (m_currentFrameTime > frametime)
    {
        m_currentFrameTime = 0.f;
        m_frameIndex = (m_frameIndex + 1) % frames.size();
        m_sprite.setTextureRect(frames[m_frameIndex]);
    }
}

//private
void LoadingScreen::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    rt.draw(m_sprite, states);
}