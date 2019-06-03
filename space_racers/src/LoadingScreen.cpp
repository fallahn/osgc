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
#include "GlobeShader.inl"
    constexpr float frametime = 1.f / 60.f;
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
}

//public
void LoadingScreen::update(float dt)
{
    m_currentFrameTime += dt;
    m_shader->setUniform("u_time", m_currentFrameTime);
}

//private
void LoadingScreen::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    rt.draw(m_backgroundSprite);
    rt.draw(m_roidSprite, m_shader.get());
}