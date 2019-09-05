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

#include "ShadowCastSystem.hpp"
#include "TorchlightSystem.hpp"
#include "GlobalConsts.hpp"
#include "Camera3D.hpp"
#include "ResourceIDs.hpp"
#include "Sprite3D.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Vector.hpp>
#include <xyginext/resources/ShaderResource.hpp>

namespace
{
    const float ShadowRadius = Global::LightRadius;// *0.89f;
    const float ShadowRadiusSqr = ShadowRadius * ShadowRadius;
}

ShadowCastSystem::ShadowCastSystem(xy::MessageBus& mb, xy::ShaderResource& sr)
    : xy::System(mb, typeid(ShadowCastSystem))
{
    requireComponent<ShadowCaster>();
    requireComponent<xy::Transform>();
    requireComponent<xy::Drawable>();

    m_shader = &sr.get(ShaderID::ShadowShader);
}

//public
void ShadowCastSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        //if a caster belongs to an entity which may get destroyed it also needs to be removed...
        const auto& caster = entity.getComponent<ShadowCaster>();
        if (!caster.parent.isValid() || caster.parent.destroyed())
        {
            getScene()->destroyEntity(entity);
            continue;
        }

        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();
        verts.clear();

        auto bounds = caster.parent.getComponent<xy::Sprite>().getTextureBounds();
        bounds = caster.parent.getComponent<xy::Transform>().getTransform().transformRect(bounds);

        auto entPos = caster.parent.getComponent<xy::Transform>().getPosition();
        entPos.y -= caster.parent.getComponent<Sprite3D>().verticalOffset;

        sf::Vector2f pointA(bounds.left, entPos.y);
        //point B is entPos
        sf::Vector2f pointC(bounds.left + bounds.width, entPos.y);

        bounds = caster.parent.getComponent<xy::Sprite>().getTextureRect();
        std::array<sf::Vector2f, 6u> texCoords = 
        {
            sf::Vector2f(bounds.left, bounds.top + bounds.height),
            sf::Vector2f(bounds.left, bounds.top),
            sf::Vector2f(bounds.left + (bounds.width / 2.f), bounds.top + bounds.height),
            sf::Vector2f(bounds.left + (bounds.width / 2.f), bounds.top),
            sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height),
            sf::Vector2f(bounds.left + bounds.width, bounds.top)
        };

        for (auto light : m_lights)
        {
            //get distance to light and skip if not nearby
            auto lightPos = light.getComponent<xy::Transform>().getPosition();
            auto lightDir2 = xy::Util::Vector::lengthSquared(lightPos - entPos);

            if (lightDir2 < ShadowRadiusSqr)
            {
                auto[rayA, shadowAmountA] = getRay(lightPos, pointA);
                auto[rayB, shadowAmountB] = getRay(lightPos, entPos);
                auto[rayC, shadowAmountC] = getRay(lightPos, pointC);

                auto lightnessA = static_cast<sf::Uint8>(255.f * shadowAmountA);
                auto lightnessB = static_cast<sf::Uint8>(255.f * shadowAmountB);
                auto lightnessC = static_cast<sf::Uint8>(255.f * shadowAmountC);

                //abs placed because no model matrix in shader
                verts.push_back({ pointA, sf::Color(lightnessA, lightnessA, lightnessA), texCoords[0] });
                verts.push_back({ pointA + rayA, sf::Color::White, texCoords[1] });
                verts.back().texCoords.y += shadowAmountA * bounds.height;

                verts.push_back({ entPos, sf::Color(lightnessB, lightnessB, lightnessB), texCoords[2] });
                verts.push_back({ entPos + rayB, sf::Color::White, texCoords[3] });
                verts.back().texCoords.y += shadowAmountB * bounds.height;

                verts.push_back({ pointC, sf::Color(lightnessC, lightnessC, lightnessC), texCoords[4] });
                verts.push_back({ pointC + rayC, sf::Color::White, texCoords[5] });
                verts.back().texCoords.y += shadowAmountC * bounds.height;
            }
        }

        drawable.updateLocalBounds();
    }

    //update from camera settings
    const auto camEnt = getScene()->getActiveCamera();
    const auto& camera = camEnt.getComponent<Camera3D>();

    auto viewProj = sf::Glsl::Mat4(&camera.viewProjectionMatrix[0][0]);
    m_shader->setUniform("u_viewProjectionMatrix", viewProj);
}

void ShadowCastSystem::addLight(xy::Entity entity)
{
    XY_ASSERT(entity.hasComponent<Torchlight>(), "Invalid entity");
    m_lights.push_back(entity);
}

void ShadowCastSystem::onEntityAdded(xy::Entity entity)
{
    entity.getComponent<xy::Drawable>().setPrimitiveType(sf::TriangleStrip);
    entity.getComponent<xy::Drawable>().setShader(m_shader);
    entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendMultiply);
    entity.getComponent<xy::Drawable>().setTexture(entity.getComponent<ShadowCaster>().parent.getComponent<xy::Sprite>().getTexture());
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Drawable>().getTexture());
}

//private
std::pair<sf::Vector2f, float> ShadowCastSystem::getRay(sf::Vector2f lightPos, sf::Vector2f basePos)
{
    auto ray = basePos - lightPos;
    auto length = xy::Util::Vector::lengthSquared(ray);
    float shadowAmount = std::min(1.f, (length / ShadowRadiusSqr) + 0.2f); //if we do this with squared lengths will we get similar falloff as light?

    length = std::sqrt(length);
    ray /= length;
    length = ShadowRadius - length;
    ray *= length;
    return std::make_pair(ray, shadowAmount);
}