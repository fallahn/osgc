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

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Shader.hpp>

#include <memory>

class LoadingScreen final : public sf::Drawable
{
public:
    LoadingScreen();

    void update(float);

private:
    sf::Texture m_backgroundTexture;
    sf::Texture m_diffuseTexture;
    sf::Texture m_normalTexture;
    sf::Sprite m_roidSprite;
    sf::Sprite m_backgroundSprite;

    //this is a kludge to prevent the LoadingScreen from becoming non-copyable
    //don't tell anyone!!
    std::shared_ptr<sf::Shader> m_shader;

    float m_currentFrameTime;

    void draw(sf::RenderTarget&, sf::RenderStates) const override;
};
