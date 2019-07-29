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

#include <vector>

class LoadingScreen final : public sf::Drawable
{
public:
    LoadingScreen();

    void update(float);

private:

    std::vector<sf::Vertex> m_vertices;

    sf::Texture m_texture;
    sf::Transform m_transform;

    float m_currentFrameTime;
    sf::Vector2f m_frameSize;
    std::size_t m_tailCount;
    static constexpr std::size_t MaxTail = 24;

    void draw(sf::RenderTarget&, sf::RenderStates) const override;
};
