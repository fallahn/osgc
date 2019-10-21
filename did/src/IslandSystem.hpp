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

/*
In charge of rendering the island ground plane
*/

#include <xyginext/ecs/System.hpp>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <array>

namespace xy
{
    class TextureResource;
    class ShaderResource;
}

struct Island final
{
    const sf::Texture* texture = nullptr; //this is fed from the island generator
    const sf::Texture* normalMap = nullptr;
    const sf::Texture* waveMap = nullptr;
};

class IslandSystem final : public xy::System, public sf::Drawable
{
public:
    IslandSystem(xy::MessageBus&, xy::TextureResource&, xy::ShaderResource&);

    void process(float) override;

    void setReflectionTexture(const sf::Texture&);

private:

    std::array<sf::Vertex, 4u> m_vertices;
    std::array<sf::Vertex, 20u> m_waveVertices;
    struct WaveQuad final
    {
        float scale = 1.f;
        float lifetime = 0.f;
    };
    std::array<WaveQuad, 3u> m_waves;
    WaveQuad m_inWave;

    sf::Shader* m_seaShader;
    sf::Shader* m_planeShader;
    sf::Shader* m_landShader;

    const sf::Texture* m_currentTexture;
    const sf::Texture* m_normalMap;
    const sf::Texture* m_waveMap;

    sf::Texture* m_seaTexture;
    sf::Transform m_seaTransform;
    mutable sf::RenderTexture m_seaBuffer;

    sf::Sprite m_reflectionSprite;
    sf::Shader* m_sunsetShader;

    void updateWaves(float);
    void draw(sf::RenderTarget&, sf::RenderStates) const override;

    void onEntityAdded(xy::Entity) override;
};