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
#include "GlobalConsts.hpp"

#include <xyginext/core/FileSystem.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include <array>

namespace
{
    constexpr float frametime = 1.f / 12.f;
    const float scale = 4.f;

    const std::int32_t FrameCountX = 4;

    const std::array<std::string, 3> randomMessages =
    {
        "Use the strafe button to face enemies",
        "Use a decoy to distract the bees",
        "Zombies are hurt by light!"
    };
}

LoadingScreen::LoadingScreen()
    : m_currentFrame    (0),
    m_currentFrameTime  (0.f)
{
    m_texture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/poopsnail.png");
    m_sprite.setTexture(m_texture);

    m_frameSize = sf::Vector2i(m_texture.getSize());
    m_frameSize /= 4;

    m_sprite.setTextureRect({ {}, m_frameSize });
    m_sprite.setScale(-scale, scale);
    m_sprite.setPosition(static_cast<float>(m_frameSize.x) * scale, 0.f);

    m_font.loadFromFile(xy::FileSystem::getResourcePath() + Global::FineFont);
    m_text.setFont(m_font);
    m_text.setFillColor(Global::InnerTextColour);
    m_text.setCharacterSize(Global::LobbyTextSize);
    updateMessage();
}

//public
void LoadingScreen::update(float dt)
{
    if (m_currentFrame < 4)
    {
        m_sprite.move(400.f * dt, 0.f);
    }

    auto oldTime = m_currentFrameTime;

    m_currentFrameTime += dt;
    if (m_currentFrameTime > frametime)
    {
        m_currentFrame = (m_currentFrame + 1) % MaxFrames;
        m_sprite.setTextureRect({ {(m_currentFrame % FrameCountX) * m_frameSize.x, (m_currentFrame / FrameCountX) * m_frameSize.y}, m_frameSize });

        m_currentFrameTime = 0.f;

        if (m_sprite.getPosition().x > xy::DefaultSceneSize.x)
        {
            m_sprite.setPosition(static_cast<float>(m_frameSize.x) * scale, 0.f);
        }
    }

    //pick a new message every 3 seconds or so
    auto prev = static_cast<std::int32_t>(oldTime) % 3;
    auto next = static_cast<std::int32_t>(m_currentFrameTime) % 3;
    if (prev == 0 && next == 1)
    {
        updateMessage();
    }
}

//private
void LoadingScreen::draw(sf::RenderTarget& rt, sf::RenderStates) const
{
    rt.draw(m_sprite);
    rt.draw(m_text);
}

void LoadingScreen::updateMessage()
{
    m_text.setString(randomMessages[xy::Util::Random::value(0, randomMessages.size() - 1)]);
    m_text.setOrigin(m_text.getLocalBounds().width / 2.f, m_text.getLocalBounds().height / 2.f);
    m_text.setPosition(xy::DefaultSceneSize.x / 2.f, 980.f);
}