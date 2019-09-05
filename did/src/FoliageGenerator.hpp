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

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <vector>
#include <memory>

struct MapData;
class MatrixPool;

namespace xy
{
    class Scene;
    class TextureResource;
    class ShaderResource;
}

namespace sf
{
    class RenderTexture;
}

struct FoliageData final
{
    float width = 0.f;
    sf::Vector2f position;
    FoliageData(float w = 0.f, sf::Vector2f p = {}) : width(w), position(p) {}
};

class FoliageGenerator final
{
public:
    FoliageGenerator(xy::TextureResource&, xy::ShaderResource&, MatrixPool&);

    void generate(const MapData&, xy::Scene&);

private:
    xy::TextureResource& m_textureResource;
    xy::ShaderResource& m_shaderResource;
    std::size_t m_textureIndex;
    std::vector<std::unique_ptr<sf::RenderTexture>> m_textures;

    std::vector<sf::Sprite> m_farSprites;
    std::vector<sf::Sprite> m_midSprites;
    std::vector<sf::Sprite> m_nearSprites;

    MatrixPool& m_modelMatrices;
};