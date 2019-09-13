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

#include "MenuState.hpp"
#include "ResourceIDs.hpp"
#include "Camera3D.hpp"
#include "GlobalConsts.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Callback.hpp>

namespace
{
    sf::Shader* seaShader = nullptr;
    sf::Shader* skyShader = nullptr;
    sf::Shader* planeShader = nullptr;

    xy::Entity camEnt;
}

void MenuState::createBackground()
{
    seaShader = &m_shaderResource.get(ShaderID::SeaShader);
    skyShader = &m_shaderResource.get(ShaderID::SkyShader);
    skyShader->setUniform("u_skyColour", sf::Glsl::Vec4(1.f, 1.f, 1.f, 1.f));
    planeShader = &m_shaderResource.get(ShaderID::PlaneShader);
    planeShader->setUniform("u_skyColour", sf::Glsl::Vec4(1.f, 1.f, 1.f, 1.f));

    //sky
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-100000);
    entity.getComponent<xy::Drawable>().setShader(skyShader);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    m_textureResource.get("assets/images/background.png").setRepeated(true);
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/background.png"));
    float scale = xy::DefaultSceneSize.x / entity.getComponent<xy::Sprite>().getTextureBounds().width;
    entity.getComponent<xy::Transform>().setScale(scale, scale);

    //sea
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-10000);
    entity.getComponent<xy::Drawable>().setShader(planeShader);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<xy::Sprite>(m_seaBuffer.getTexture());

    //island
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setScale(2.f, 2.f);
    entity.addComponent<xy::Drawable>().setDepth(-9999);
    entity.getComponent<xy::Drawable>().setShader(planeShader);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/menu_island.png"));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setPosition(Global::IslandSize.x / 2.f, Global::IslandSize.y * 0.4f);
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [](xy::Entity e, float dt)
    {
        e.getComponent<xy::Transform>().rotate(3.f * dt);
    };

    //camera
    camEnt = m_gameScene.createEntity();
    camEnt.addComponent<xy::Transform>();
    const float fov = 0.6f; //0.55f FOV
    const float aspect = 16.f / 9.f;
    camEnt.addComponent<Camera3D>().projectionMatrix = glm::perspective(fov, aspect, 0.1f, 1536.f);
    camEnt.getComponent<Camera3D>().pitch = -0.18f;
    camEnt.getComponent<Camera3D>().height = 200.f;
    camEnt.getComponent<xy::Transform>().setPosition(Global::IslandSize.x / 2.f, Global::IslandSize.y - 140.f);
}

void MenuState::updateBackground(float dt)
{
    static float shaderTime = 0.f;
    shaderTime += dt;

    seaShader->setUniform("u_time", shaderTime); //can't mod the time here else it messes with other parts of the shader
    skyShader->setUniform("u_time", shaderTime * 0.03f);

    //update 3D parts
    const auto& camera = camEnt.getComponent<Camera3D>();

    auto viewProj = sf::Glsl::Mat4(&camera.viewProjectionMatrix[0][0]);
    auto worldCam = sf::Glsl::Vec3(camera.worldPosition.x, camera.worldPosition.y, camera.worldPosition.z);
    planeShader->setUniform("u_viewProjection", viewProj);
    planeShader->setUniform("u_cameraWorldPosition", worldCam);
}