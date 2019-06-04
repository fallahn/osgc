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

//encapsulates the rendering path for multi-pass processes - simpler to shared between game modes

#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Drawable.hpp>

#include <functional>

namespace xy
{
    class Scene;
    class ResourceHandler;
}

class RenderPath final : public sf::Drawable
{
public:
    explicit RenderPath(xy::ResourceHandler&);

    bool init(bool);

    void updateView(sf::Vector2f);

    std::function<void(xy::Scene&, xy::Scene&)> render;

    const sf::Texture& getNormalBuffer() const { return m_normalBuffer.getTexture(); }
    const sf::Texture& getBackgroundBuffer() const { return m_backgroundBuffer.getTexture(); }

    void setNormalTexture(const sf::Texture& t) { m_normalSprite.setTexture(t, true); }

private:

    sf::RenderTexture m_backgroundBuffer;
    sf::Sprite m_backgroundSprite;

    sf::RenderTexture m_normalBuffer;
    sf::Sprite m_normalSprite;

    sf::RenderTexture m_neonBuffer;
    sf::RenderTexture m_blurBuffer;
    sf::Sprite m_neonSprite;

    sf::RenderTexture m_gameSceneBuffer;
    sf::Sprite m_gameSceneSprite;

    xy::ShaderResource m_shaders;

    void renderPretty(xy::Scene&, xy::Scene&);
    void renderBasic(xy::Scene&, xy::Scene&);

    sf::Shader* m_blendShader;
    void draw(sf::RenderTarget&, sf::RenderStates) const override;
};