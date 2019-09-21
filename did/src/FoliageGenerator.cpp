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

#include "FoliageGenerator.hpp"
#include "IslandGenerator.hpp"
#include "Sprite3D.hpp"
#include "CommandIDs.hpp"
#include "ResourceIDs.hpp"
#include "Camera3D.hpp"
#include "GlobalConsts.hpp"
#include "MatrixPool.hpp"
#include "FoliageSystem.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/resources/Resource.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Graphics/RenderTexture.hpp>

namespace
{
    const std::size_t MaxTextures = Global::MaxFoliage;
    const sf::Uint32 FoliageHeight = 256u;// 128u;
    const sf::Vector2f FarScale(0.87f, 0.87f);
    const sf::Vector2f NearScale(0.6f, 0.6f);
}

FoliageGenerator::FoliageGenerator(xy::TextureResource& tr, xy::ShaderResource& sr, MatrixPool& mp)
    : m_textureResource (tr),
    m_shaderResource    (sr),
    m_textureIndex      (0),
    m_modelMatrices     (mp)
{
    m_textures.emplace_back();

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/foliage.spt", tr);

    const auto& sprites = spriteSheet.getSprites();
    for (auto& spr : sprites)
    {
        sf::Sprite foliageSprite;
        foliageSprite.setTexture(tr.get(spriteSheet.getTexturePath()));
        foliageSprite.setTextureRect(sf::IntRect(spr.second.getTextureRect()));
        foliageSprite.setOrigin(0.f, spr.second.getTextureRect().height);

        if (spr.first.find("far") != std::string::npos)
        {
            foliageSprite.setScale(FarScale);
            m_farSprites.push_back(foliageSprite);
        }
        else if (spr.first.find("mid") != std::string::npos)
        {
            m_midSprites.push_back(foliageSprite);
        }
        else
        {
            foliageSprite.setScale(NearScale);
            m_nearSprites.push_back(foliageSprite);
        }
    }
}

//public
void FoliageGenerator::generate(const MapData& data, xy::Scene& scene)
{
    m_textureIndex = 0;

    for (auto i = 0u; i < data.foliageCount; ++ i)
    {
        const auto& f = data.foliageData[i];

        //draw a variety of foliages making sure they fit on the texture
        auto drawFoliage = [&](const std::vector<sf::Sprite>& sprites, float spacing = 1.2f)
        {
            const float width = f.width * 2.f;

            if (sprites.empty())
            {
                return;
            }

            sf::Vector2f position(0.f, static_cast<float>(FoliageHeight));
            bool passComplete = false;
            for (auto i = 0; i < 2; ++i)
            {
                while (!passComplete)
                {
                    std::size_t spriteIndex = xy::Util::Random::value(0, sprites.size() - 1);
                    auto sprite = sprites[spriteIndex];

                    sf::FloatRect spriteRect = sprite.getLocalBounds();

                    //check we didn't start this pass too far to the end
                    if (position.x + spriteRect.width > width)
                    {
                        position.x = std::max(0.f, width - spriteRect.width);
                    }

                    //if sprite is too wide then scale
                    auto currScale = sprite.getScale();
                    if (spriteRect.width > width)
                    {
                        float scale = width / spriteRect.width;
                        sprite.setScale(scale * currScale.x, scale * currScale.y);
                    }

                    //actually draw to texture
                    sprite.setPosition(position);
                    m_textures[m_textureIndex]->draw(sprite);
                    position.x += (spriteRect.width * spacing) + xy::Util::Random::value(-30.f * spacing, -10.f * spacing);

                    //if we got to the edge tidy up current pass
                    if (position.x + spriteRect.width > width)
                    {
                        position.x = std::max(0.f, width - spriteRect.width);
                        sprite.setPosition(position);
                        if(i==0)m_textures[m_textureIndex]->draw(sprite);
                        passComplete = true;

                        //rewind to the beginning
                        position.x = spriteRect.width * 1.25f;
                    }
                }
                passComplete = false;
            }
        };

        m_textures[m_textureIndex] = std::make_unique<sf::RenderTexture>();
        m_textures[m_textureIndex]->create(static_cast<sf::Uint32>(f.width * 2.f), FoliageHeight);
        m_textures[m_textureIndex]->clear(sf::Color::Transparent);

        drawFoliage(m_farSprites, 0.8f);

        //mid sprites are less frequent, such as heads or gravestones
        if (!m_midSprites.empty())
        {
            if (xy::Util::Random::value(0, 6) == 0)
            {
                auto count = (f.width > (Global::TileSize * 2.5f)) ? xy::Util::Random::value(1, 2) : 1;
                for (auto i = 0; i < count; ++i)
                {
                    auto& sprite = m_midSprites[xy::Util::Random::value(0, m_midSprites.size() - 1)];
                    sf::FloatRect spriteRect = sprite.getLocalBounds();
                    sf::Vector2f position(static_cast<float>(xy::Util::Random::value(0, static_cast<int>((f.width * 2.f) - spriteRect.width))), static_cast<float>(FoliageHeight));
                    sprite.setPosition(position);
                    m_textures[m_textureIndex]->draw(sprite);
                }
            }
        }

        drawFoliage(m_nearSprites);

        m_textures[m_textureIndex]->display();
        
        const auto& cameraComponent = scene.getActiveCamera().getComponent<Camera3D>();

        //remember we have to flip the sprites
        auto entity = scene.createEntity();
        entity.addComponent<Sprite3D>(m_modelMatrices);
        entity.addComponent<xy::Sprite>().setTexture(m_textures[m_textureIndex]->getTexture());
        entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShader));
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraComponent.viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", m_textures[m_textureIndex]->getTexture());
        entity.getComponent<xy::Drawable>().setCulled(false);
        entity.addComponent<xy::Transform>().setPosition(f.position);
        entity.getComponent<xy::Transform>().move(Global::TileSize / 4.f, 0.f);
        entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f); //texture causes this to be double size
        //entity.getComponent<xy::Transform>().setScale(1.f, -1.f);

        //wavy trees:
        entity = scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(f.position);
        entity.getComponent<xy::Transform>().move(40.f, -0.1f);
        entity.addComponent<xy::Drawable>().setPrimitiveType(sf::TriangleStrip);

        entity.addComponent<Sprite3D>(m_modelMatrices).needsCorrection = false;
        entity.getComponent<xy::Drawable>().setTexture(&m_textureResource.get("assets/images/palm_tree.png"));
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShader/*Culled*/));
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraComponent.viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Drawable>().getTexture());
        entity.addComponent<Tree>().width = f.width;

        m_textureIndex++;
        if (m_textureIndex == MaxTextures)
        {
            break;
        }

        else if (m_textureIndex == m_textures.size())
        {
            m_textures.resize(m_textures.size() + 10);
        }
    }
}