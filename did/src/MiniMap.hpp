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

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/CircleShape.hpp> //TODO could consolidate these into a vert array
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <vector>
#include <array>

namespace xy
{
    class TextureResource;
    class Message;
}

class MiniMap final
{
public:
    explicit MiniMap(xy::TextureResource&);

    //set the island map texture
    void setTexture(const sf::Texture&);

    //get the rendered output
    const sf::Texture& getTexture() const;

    void handleMessage(const xy::Message&);

    void updateTexture();

    void update();

    void setLocalPlayer(std::size_t);

    void addCross(sf::Vector2f);

private:

    xy::TextureResource& m_textureResource;
    std::array<sf::CircleShape, 4u> m_playerPoints;
    sf::CircleShape m_halo;
    std::size_t m_localPlayer;

    sf::RenderTexture m_outputBuffer;
    sf::RenderTexture m_backgroundTexture;
    //sf::RenderTexture m_fowTexture;

    sf::Shader m_mapShader;

    sf::Texture* m_crossTexture;
    std::vector<sf::Vertex> m_crossVerts;

    sf::Sprite m_backgroundSprite;
};