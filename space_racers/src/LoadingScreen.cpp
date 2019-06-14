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
#include "ResourceIDs.hpp"

#include <xyginext/core/FileSystem.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include <array>

namespace
{
#include "GlobeShader.inl"
    constexpr float frametime = 1.f / 60.f;

    const std::array<std::string, 20> randomMessages = 
    {
        "exec COMMAND.COM",
        "Smuggling Raisins since 1903",
        "Pay no attention to the man behind the curtain",
        "Who cut the cheese?",
        "You are not reading this",
        "Always assume a wink is suspicious",
        "Mauve is the new purple",
        "Pet your kitty",
        "There are no pork pies in the Netherlands",
        "Sponsored by Powdered Toast",
        "Please stop looking at me",
        "Now with added Bosons!",
        "Goes great with Salmon",
        "Wouldn't you like to be a racer too?",
        "Not tested on animals",
        "Any bugs you may encounter should be considered a feature",
        "You may have read this before",
        "The chances are the odds of something happening",
        "Mustard is underrated",
        "Nothing up our sleeves" 
    };
}

LoadingScreen::LoadingScreen()
    : m_currentFrameTime  (0.f)
{
    m_diffuseTexture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/roid_diffuse.png");
    m_diffuseTexture.setSmooth(true);
    m_diffuseTexture.setRepeated(true);

    m_normalTexture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/crater_normal.png");
    m_normalTexture.setRepeated(true);
    
    m_shader = std::make_unique<sf::Shader>();
    m_shader->loadFromMemory(GlobeFragment, sf::Shader::Fragment);

    m_roidSprite.setTexture(m_diffuseTexture, true);
    m_roidSprite.setOrigin(sf::Vector2f(m_diffuseTexture.getSize()) / 2.f);
    m_roidSprite.setPosition(xy::DefaultSceneSize / 2.f);
    m_roidSprite.setScale(2.f, 2.f);

    m_shader->setUniform("u_texture", m_diffuseTexture);
    m_shader->setUniform("u_normalMap", m_normalTexture);

    m_backgroundTexture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/stars.png");
    m_backgroundSprite.setTexture(m_backgroundTexture, true);

    m_font.loadFromFile(xy::FileSystem::getResourcePath() + "assets/fonts/" + FontID::DefaultFont);
    m_text.setFont(m_font);
    updateMessage();
}

//public
void LoadingScreen::update(float dt)
{
    float oldTime = m_currentFrameTime;

    m_currentFrameTime += dt;
    m_shader->setUniform("u_time", m_currentFrameTime);

    //pick a new message every 3 seconds or so
    auto prev = static_cast<std::int32_t>(oldTime) % 3;
    auto next = static_cast<std::int32_t>(m_currentFrameTime) % 3;
    if (prev == 0 && next == 1)
    {
        updateMessage();
    }
}

//private
void LoadingScreen::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    rt.draw(m_backgroundSprite);
    rt.draw(m_roidSprite, m_shader.get());
    rt.draw(m_text);
}

void LoadingScreen::updateMessage()
{
    m_text.setString(randomMessages[xy::Util::Random::value(0, randomMessages.size() - 1)]);
    m_text.setOrigin(m_text.getLocalBounds().width / 2.f, m_text.getLocalBounds().height / 2.f);
    m_text.setPosition(xy::DefaultSceneSize.x / 2.f, 980.f);
}