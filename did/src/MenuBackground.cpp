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
#include "WaveSystem.hpp"
#include "Sprite3D.hpp"
#include "FlappySailSystem.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>

namespace
{
    sf::Shader* seaShader = nullptr;
    sf::Shader* skyShader = nullptr;
    sf::Shader* planeShader = nullptr;
    sf::Shader* landShader = nullptr;

    xy::Entity camEnt;

    std::array<sf::FloatRect, 4u> sailColours =
    {
        sf::FloatRect(80.f, 216.f, 49.f, 60.f),
        { 131.f, 216.f, 49.f, 60.f },
        { 182.f, 216.f, 49.f, 60.f },
        { 80.f, 278.f, 49.f, 60.f }
    };
}

void MenuState::createBackground()
{
    seaShader = &m_shaderResource.get(ShaderID::SeaShader);
    skyShader = &m_shaderResource.get(ShaderID::SkyShader);
    skyShader->setUniform("u_skyColour", sf::Glsl::Vec4(1.f, 1.f, 1.f, 1.f));
    planeShader = &m_shaderResource.get(ShaderID::PlaneShader);
    planeShader->setUniform("u_skyColour", sf::Glsl::Vec4(1.f, 1.f, 1.f, 1.f));
    landShader = &m_shaderResource.get(ShaderID::LandShader);
    landShader->setUniform("u_skyColour", sf::Glsl::Vec4(1.f, 1.f, 1.f, 1.f));
    landShader->setUniform("u_sunDirection", sf::Glsl::Vec3(1.f, 1.f, 1.f));
    landShader->setUniform("u_normalTexture", m_textureResource.get("assets/images/menu_island_normal.png"));

    m_shaderResource.get(ShaderID::SpriteShader).setUniform("u_lightAmount", 1.f);
    m_shaderResource.get(ShaderID::SpriteShader).setUniform("u_skyColour", sf::Glsl::Vec4(1.f, 1.f, 1.f, 1.f));

    //camera
    camEnt = m_gameScene.createEntity();
    camEnt.addComponent<xy::Transform>();
    const float fov = 0.6f; //0.55f FOV
    const float aspect = 16.f / 9.f;
    camEnt.addComponent<Camera3D>().projectionMatrix = glm::perspective(fov, aspect, 0.1f, 1536.f);
    camEnt.getComponent<Camera3D>().pitch = -0.18f;
    camEnt.getComponent<Camera3D>().height = 200.f;
    camEnt.getComponent<xy::Transform>().setPosition(Global::IslandSize.x / 2.f, Global::IslandSize.y - 140.f);

    auto view = getContext().defaultView;
    auto& camera = camEnt.addComponent<xy::Camera>();
    camera.setView(view.getSize());
    camera.setViewport(view.getViewport());

    m_gameScene.setActiveCamera(camEnt);

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
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-9998);
    entity.getComponent<xy::Drawable>().setShader(landShader);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseTexture");
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
    //this is used below to update positions of island items
    const auto& islandTx = entity.getComponent<xy::Transform>();


    //waves
    std::array<float, 3> offsets =
    {
        0.f, ((Wave::MaxScale - Wave::MaxScale) * 0.4f),
        ((Wave::MaxScale - Wave::MinScale) * 0.7f)
    };
    for (auto i = 0; i < 3; ++i)
    {
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>();
        entity.addComponent<xy::Drawable>().setDepth(-9999);
        entity.getComponent<xy::Drawable>().setShader(planeShader);
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/menu_island_waves.png"));
        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setPosition(Global::IslandSize.x / 2.f, Global::IslandSize.y * 0.4f);
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        entity.addComponent<Wave>().currentScale = Wave::MinScale + offsets[i];
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [](xy::Entity e, float dt)
        {
            e.getComponent<xy::Transform>().rotate(3.f * dt);
        };
    }

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/barrel.spt", m_textureResource);

    //barrels
    std::array<sf::Vector2f, 8u> barrelPos =
    {
        sf::Vector2f(762.f, 455.f),
        {1066.f, 597.f},
        {1048.f, 807.f},
        {759.f, 1044.f},
        {478.f, 667.f},
        {469.f, 644.f},
        {978.f, 527.f},
        {477.f, 924.f}
    };

    for (auto i = 0u; i < 8u; ++i)
    {
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(barrelPos[i]);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("barrel01");
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShader));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool);
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &camEnt.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);

        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);

        auto rootPos = barrelPos[i] - (Global::IslandSize / 2.f);
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [rootPos, &islandTx](xy::Entity e, float)
        {
            sf::Transform tx;
            tx.translate(islandTx.getPosition());
            tx.rotate(islandTx.getRotation());

            e.getComponent<xy::Transform>().setPosition(tx.transformPoint(rootPos));
        };
    }

    //boats
    std::array<sf::Vector2f, 4u> boatPos =
    {
        sf::Vector2f(462.f, 455.f),
        {1066.f, 457.f},
        {978.f, 1007.f},
        {459.f, 1044.f}
    };

    spriteSheet.loadFromFile("assets/sprites/boat.spt", m_textureResource);
    for (auto i = 0; i < 4; ++i)
    {
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(boatPos[i]);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("boat");
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShader));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool);
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &camEnt.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);

        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);

        auto rootPos = boatPos[i] - (Global::IslandSize / 2.f);
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [rootPos, &islandTx](xy::Entity e, float)
        {
            sf::Transform tx;
            tx.translate(islandTx.getPosition());
            tx.rotate(islandTx.getRotation());

            e.getComponent<xy::Transform>().setPosition(tx.transformPoint(rootPos));
        };

        //add the sails
        auto sailEnt = m_gameScene.createEntity();
        sailEnt.addComponent<xy::Transform>().setPosition(50.f, 0.01f);
        sailEnt.getComponent<xy::Transform>().setScale(0.5f, 0.5f);
        sailEnt.addComponent<xy::Drawable>().setTexture(spriteSheet.getSprite("boat").getTexture());
        sailEnt.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShader));
        sailEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");

        sailEnt.addComponent<FlappySail>().clothRect = sailColours[i];
        sailEnt.getComponent<FlappySail>().poleRect = { 0.f, 216.f, 74.f, 64.f };

        sailEnt.addComponent<Sprite3D>(m_matrixPool).needsCorrection = false;
        sailEnt.getComponent<Sprite3D>().verticalOffset = -10.2f;
        sailEnt.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &camEnt.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        sailEnt.getComponent<xy::Drawable>().bindUniform("u_modelMat", &sailEnt.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Transform>().addChild(sailEnt.getComponent<xy::Transform>());
    }

    //finally.... some trees!
    const auto& treeTex = m_textureResource.get("assets/images/menu_trees.png");
    auto positions = xy::Util::Random::poissonDiscDistribution({ 0.f, 0.f, 440.f, 430.f }, 60.f, 12);

    spriteSheet.loadFromFile("assets/sprites/foliage.spt", m_textureResource);

    for (auto pos : positions)
    {
        pos.x += 526.f;
        pos.y += 526.f;

        auto head = xy::Util::Random::value(0, 10);

        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(pos);
        entity.addComponent<xy::Drawable>();
        if (head == 0)
        {
            if (xy::Util::Random::value(0, 1) == 0)
            {
                entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("mid_head");
            }
            else
            {
                entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("mid_grave");
            }
        }
        else
        {
            entity.addComponent<xy::Sprite>(treeTex);
        }
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShader));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool);
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &camEnt.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        
        float scaleMod = xy::Util::Random::value(0.05f, 0.28f);
        float treeScale = 0.3f + scaleMod;
        entity.getComponent<xy::Transform>().setScale(treeScale, -treeScale);
        if ((xy::Util::Random::value(0, 10) % 3) == 0)
        {
            entity.getComponent<xy::Transform>().scale(-1.f, 1.f);
        }

        bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        if (head != 0)
        {
            bounds.width /= 3.f;
            auto offset = xy::Util::Random::value(0, 2);
            bounds.left = offset * bounds.width;
            entity.getComponent<xy::Sprite>().setTextureRect(bounds);
        }

        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);

        auto rootPos = pos - (Global::IslandSize / 2.f);
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [rootPos, &islandTx](xy::Entity e, float)
        {
            sf::Transform tx;
            tx.translate(islandTx.getPosition());
            tx.rotate(islandTx.getRotation());

            e.getComponent<xy::Transform>().setPosition(tx.transformPoint(rootPos));
        };
    }
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

    landShader->setUniform("u_viewProjection", viewProj);
    landShader->setUniform("u_cameraWorldPosition", worldCam);
}