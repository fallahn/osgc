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

#include "IslandSystem.hpp"
#include "Camera3D.hpp"
#include "GlobalConsts.hpp"
#include "ResourceIDs.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Transformable.hpp>

#include "glm/gtc/matrix_transform.hpp"

namespace
{
    const float MaxWaveSize = 1.05f;
    const float WaveLifetime = 2.f;

    const float ReflectionOffset = 60.f;
}

IslandSystem::IslandSystem(xy::MessageBus& mb, xy::TextureResource& tr, xy::ShaderResource& sr)
    : xy::System    (mb, typeid(IslandSystem)),
    m_seaShader     (nullptr),
    m_planeShader   (nullptr),
    m_landShader    (nullptr),
    m_currentTexture(nullptr),
    m_normalMap     (nullptr),
    m_waveMap       (nullptr),
    m_seaTexture    (nullptr),
    m_sunsetShader  (nullptr)
{
    m_vertices[1].position = { Global::IslandSize.x, 0.f };
    m_vertices[1].texCoords = m_vertices[1].position;

    m_vertices[2].position = Global::IslandSize;
    m_vertices[2].texCoords = Global::IslandSize;

    m_vertices[3].position = { 0.f, Global::IslandSize.y };
    m_vertices[3].texCoords = m_vertices[3].position;

    m_seaBuffer.create(static_cast<unsigned>(Global::IslandSize.x), static_cast<unsigned>(Global::IslandSize.y));

    m_seaTexture = &tr.get("assets/images/sea.png");
    m_seaTexture->setRepeated(true);
    sr.get(ShaderID::SeaShader).setUniform("u_texture", *m_seaTexture);
    
    m_seaShader = &sr.get(ShaderID::SeaShader);
    m_planeShader = &sr.get(ShaderID::PlaneShader);
    m_landShader = &sr.get(ShaderID::LandShader);


    const float spacing = WaveLifetime / m_waves.size();
    for (auto i = 0u; i < m_waves.size(); ++i)
    {
        m_waves[i].lifetime += spacing * static_cast<float>(i);
        auto j = i * 4;
        m_waveVertices[j].texCoords = {};
        m_waveVertices[j + 1].texCoords = { Global::IslandSize.x/* / 4.f*/, 0.f };
        m_waveVertices[j + 2].texCoords = { Global::IslandSize /*/ 4.f*/ };
        m_waveVertices[j + 3].texCoords = { 0.f, Global::IslandSize.y /*/ 4.f*/ };
    }

    //one extra quad for shore/sand wave
    m_waveVertices[16].texCoords = {};
    m_waveVertices[17].texCoords = { Global::IslandSize.x, 0.f };
    m_waveVertices[18].texCoords = { Global::IslandSize  };
    m_waveVertices[29].texCoords = { 0.f, Global::IslandSize.y };

    m_sunsetShader = &sr.get(ShaderID::SunsetShader);

    requireComponent<Island>();
}

//public
void IslandSystem::process(float dt)
{
    //update sea shader
    static float timeAccumulator = 0.f;
    timeAccumulator += dt;
    m_seaShader->setUniform("u_time", timeAccumulator);

    //update from camera settings
    const auto camEnt = getScene()->getActiveCamera();
    const auto& camera = camEnt.getComponent<Camera3D>();

    //auto view = sf::Glsl::Mat4(&camera.viewMatrix[0][0]);
    auto viewProj = sf::Glsl::Mat4(&camera.viewProjectionMatrix[0][0]);
    auto worldCam = sf::Glsl::Vec3(camera.worldPosition.x, camera.worldPosition.y, camera.worldPosition.z);
    m_planeShader->setUniform("u_viewProjection", viewProj);
    m_planeShader->setUniform("u_cameraWorldPosition", worldCam);
    
    //m_landShader->setUniform("u_view", view);
    m_landShader->setUniform("u_viewProjection", viewProj);
    m_landShader->setUniform("u_cameraWorldPosition", worldCam);
    

    //make the sea follow so it seems infinite :D
    auto camPos = camEnt.getComponent<xy::Transform>().getWorldPosition();
    m_seaTransform = sf::Transform::Identity;
    m_seaTransform.translate(camPos);
    m_seaTransform.translate(-((Global::IslandSize.x / 2.f) + 100.f), -Global::IslandSize.y); //this is just stretched a bit up to the screen edge
    m_seaTransform.scale(1.2f, 1.f);
    auto textureOffset = sf::Vector2f(camPos.x / Global::IslandSize.x, camPos.y / Global::IslandSize.y);
    m_seaShader->setUniform("u_textureOffset", textureOffset);

    camPos.y -= ReflectionOffset;
    m_reflectionSprite.setPosition(camPos);

    timeAccumulator += dt;
    m_sunsetShader->setUniform("u_time", timeAccumulator);

    updateWaves(dt);
}

void IslandSystem::setReflectionTexture(const sf::Texture& texture)
{
    m_reflectionSprite.setTexture(texture);
    m_reflectionSprite.setTextureRect({ 0, 0, 1920, 540 - static_cast<int>(ReflectionOffset) });
    m_reflectionSprite.setOrigin(xy::DefaultSceneSize.x / 2.f, (xy::DefaultSceneSize.y / 2.f) - ReflectionOffset);
    m_reflectionSprite.setScale(1.f, -1.f);
    //m_reflectionSprite.setColor({ 240, 240, 240, 110 });

    m_sunsetShader->setUniform("u_texture", texture);
}

//private
void IslandSystem::updateWaves(float dt)
{
    sf::Transformable transformable;
    transformable.setOrigin(Global::IslandSize / 2.f);
    transformable.setPosition(Global::IslandSize / 2.f);

    auto timeInc = dt * 0.05f;

    //growing waves
    for (auto i = 0u; i < m_waves.size(); ++i)
    {
        transformable.setScale(m_waves[i].scale, m_waves[i].scale);
        const auto& tx = transformable.getTransform();

        float alpha = 1.f - ((m_waves[i].scale - 1.f) / (MaxWaveSize - 1.f));
        alpha = std::min(std::max(0.f, alpha), 1.f);

        m_waves[i].scale += timeInc *(alpha + 0.1f);
        auto j = i * 4;
        m_waveVertices[j].position = tx.transformPoint({});
        m_waveVertices[j + 1].position = tx.transformPoint({ Global::IslandSize.x, 0.f });
        m_waveVertices[j + 2].position = tx.transformPoint(Global::IslandSize);
        m_waveVertices[j + 3].position = tx.transformPoint({ 0.f, Global::IslandSize.y });

        alpha *= 255.f;
        sf::Color colour(255, 255, 255, static_cast<sf::Uint8>(alpha));
        m_waveVertices[j].color = colour;
        m_waveVertices[j+1].color = colour;
        m_waveVertices[j+2].color = colour;
        m_waveVertices[j+3].color = colour;

        m_waves[i].lifetime -= dt;
        if (/*m_waves[i].scale > MaxWaveSize
            &&*/ m_waves[i].lifetime < 0)
        {
            m_waves[i].scale = 1.f;
            m_waves[i].lifetime = WaveLifetime;
        }
    }

    static const float MinWaveSize = 0.03f;
    //shrinking shore wave
    transformable.setScale(m_inWave.scale, m_inWave.scale);
    const auto& tx = transformable.getTransform();

    float alpha = 1.f - ((1.f - m_inWave.scale) / MinWaveSize);
    alpha = std::min(std::max(0.f, alpha), 1.f);
    m_inWave.scale -= timeInc *(alpha + 0.01f);

    m_waveVertices[16].position = tx.transformPoint({});
    m_waveVertices[17].position = tx.transformPoint({ Global::IslandSize.x, 0.f });
    m_waveVertices[18].position = tx.transformPoint(Global::IslandSize);
    m_waveVertices[19].position = tx.transformPoint({ 0.f, Global::IslandSize.y });

    alpha *= 63.f;
    sf::Color colour(255, 255, 255, static_cast<sf::Uint8>(alpha));
    m_waveVertices[16].color = colour;
    m_waveVertices[17].color = colour;
    m_waveVertices[18].color = colour;
    m_waveVertices[19].color = colour;

    m_inWave.lifetime -= dt;
    if (m_inWave.lifetime < 0)
    {
        m_inWave.lifetime = WaveLifetime / 2.f;
        m_inWave.scale = 1.f;
    }
}

void IslandSystem::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    //render to buffer
    m_seaBuffer.clear(sf::Color(0, 77, 179));
    states.shader = m_seaShader;
    states.texture = m_seaTexture;
    m_seaBuffer.draw(m_vertices.data(), m_vertices.size(), sf::Quads, states);
    m_seaBuffer.display();


    //render to output
    //sea
    states.texture = &m_seaBuffer.getTexture();
    states.shader = m_planeShader;
    states.transform = m_seaTransform;
    rt.draw(m_vertices.data(), m_vertices.size(), sf::Quads, states);

    //sun/moon reflection
    sf::RenderStates refStates;
    refStates.shader = m_sunsetShader;
    refStates.blendMode = sf::BlendAdd;
    rt.draw(m_reflectionSprite, refStates);

    //ground
    m_landShader->setUniform("u_diffuseTexture", *m_currentTexture);
    m_landShader->setUniform("u_normalTexture", *m_normalMap);
    m_landShader->setUniform("u_normalUVOffset", 0.f);
    states.transform = {};
    states.texture = m_currentTexture;
    states.shader = m_landShader;
    states.blendMode = sf::BlendAlpha;
    rt.draw(m_vertices.data(), m_vertices.size(), sf::Quads, states);

    //waves
    states.shader = m_planeShader;
    states.texture = m_waveMap;
    states.blendMode = sf::BlendAdd;
    rt.draw(m_waveVertices.data(), m_waveVertices.size(), sf::Quads, states);

}

void IslandSystem::onEntityAdded(xy::Entity entity)
{
    m_currentTexture = entity.getComponent<Island>().texture;
    m_normalMap = entity.getComponent<Island>().normalMap;
    m_waveMap = entity.getComponent<Island>().waveMap;

    XY_ASSERT(m_currentTexture, "missing texture");
    XY_ASSERT(m_normalMap, "missing normal map");
    XY_ASSERT(m_waveMap, "missing wavemap texture");

    //m_landShader->setUniform("u_diffuseTexture", *m_currentTexture);
    //m_landShader->setUniform("u_normalTexture", *m_normalMap);
}