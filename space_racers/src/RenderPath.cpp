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

#include "RenderPath.hpp"
#include "GameConsts.hpp"
#include "ResourceIDs.hpp"
#include "Camera3D.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <SFML/OpenGL.hpp>
#include <SFML/System/Clock.hpp>

namespace
{
#include "BlurShader.inl"
#include "NeonBlendShader.inl"

    sf::Clock shaderClock;
}

RenderPath::RenderPath(xy::ResourceHandler& rh, xy::ShaderResource& sr)
    : m_shaders     (sr),
    m_blendShader   (nullptr)
{
    auto stars = rh.load<sf::Texture>("assets/images/stars.png");
    m_backgroundSprite.setTexture(rh.get<sf::Texture>(stars), true);
}

bool RenderPath::init(bool useBloom)
{
    if (!m_backgroundBuffer.create(GameConst::LargeBufferSize.x, GameConst::LargeBufferSize.y))
    {
        xy::Logger::log("Failed creating background buffer", xy::Logger::Type::Error);
        return false;
    }

    if (!m_gameSceneBuffer.create(GameConst::LargeBufferSize.x, GameConst::LargeBufferSize.y, sf::ContextSettings(8u)))
    {
        xy::Logger::log("Failed creating game scene buffer", xy::Logger::Type::Error);
        return false;
    }
    else
    {
        m_gameSceneSprite.setTexture(m_gameSceneBuffer.getTexture(), true);
    }

    if (useBloom)
    {
        if (!m_normalBuffer.create(GameConst::SmallBufferSize.x, GameConst::SmallBufferSize.y))
        {
            xy::Logger::log("Failed creating normal buffer", xy::Logger::Type::Error);
            return false;
        }
        else
        {
            m_normalBuffer.setSmooth(true);
        }

        if (!m_neonBuffer.create(GameConst::SmallBufferSize.x, GameConst::SmallBufferSize.y))
        {
            xy::Logger::log("Failed creating neon buffer", xy::Logger::Type::Error);
            return false;
        }
        else
        {
            m_neonBuffer.setSmooth(true);
        }

        if (!m_blurBuffer.create(GameConst::SmallBufferSize.x, GameConst::SmallBufferSize.y))
        {
            xy::Logger::log("Failed creating blur buffer", xy::Logger::Type::Error);
            return false;
        }

        m_shaders.preload(ShaderID::Blur, BlurFragment, sf::Shader::Fragment);
        m_shaders.preload(ShaderID::NeonBlend, BlendFragment, sf::Shader::Fragment);
        m_shaders.preload(ShaderID::NeonExtract, ExtractFragment, sf::Shader::Fragment);

        auto& blendShader = m_shaders.get(ShaderID::NeonBlend);
        blendShader.setUniform("u_sceneTexture", m_gameSceneBuffer.getTexture());
        blendShader.setUniform("u_neonTexture", m_neonBuffer.getTexture());

        auto& extractShader = m_shaders.get(ShaderID::NeonExtract);
        extractShader.setUniform("u_texture", m_gameSceneBuffer.getTexture());

        m_blendShader = &m_shaders.get(ShaderID::NeonBlend);

        render = std::bind(&RenderPath::renderPretty, this, std::placeholders::_1, std::placeholders::_2);
    }
    else
    {
        render = std::bind(&RenderPath::renderBasic, this, std::placeholders::_1, std::placeholders::_2);
    }
    return true;
}

//public
void RenderPath::updateView(sf::Vector2f camPosition)
{
    auto view = m_backgroundBuffer.getDefaultView();
    view.setCenter(camPosition);
    m_normalBuffer.setView(view);

    //m_blendShader->setUniform("u_time", shaderClock.getElapsedTime().asSeconds());
}

//private
void RenderPath::renderPretty(xy::Scene& backgroundScene, xy::Scene& gameScene)
{
    if (m_cameras.empty())
    {
        //draw the background first for distort effect
        m_backgroundBuffer.setView(m_backgroundBuffer.getDefaultView());
        m_backgroundBuffer.clear();
        m_backgroundBuffer.draw(m_backgroundSprite);
        m_backgroundBuffer.draw(backgroundScene);
        m_backgroundBuffer.display();

        //normal map for distortion of background
        m_normalBuffer.clear({ 127,127,255 });
        m_normalBuffer.draw(m_normalSprite);
        m_normalBuffer.display();
    }

    //draws the full scene including distorted background
    auto& shader = m_shaders.get(ShaderID::Sprite3DTextured);

    m_gameSceneBuffer.setActive(true);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_cameras.empty())
    {
        auto cam = gameScene.getActiveCamera();
        shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));
        m_gameSceneBuffer.draw(gameScene);
    }
    else
    {
        m_gameSceneBuffer.setView(m_gameSceneBuffer.getDefaultView());
        m_gameSceneBuffer.draw(m_backgroundSprite);

        for (auto cam : m_cameras)
        {
            shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));

            gameScene.setActiveCamera(cam);
            m_gameSceneBuffer.draw(gameScene);
        }
    }
    m_gameSceneBuffer.display();

    //extract the brightest points to the neon buffer
    auto& extractShader = m_shaders.get(ShaderID::NeonExtract);
    m_neonSprite.setTexture(m_gameSceneBuffer.getTexture(), true);
    m_neonBuffer.setView(m_gameSceneBuffer.getDefaultView());
    m_neonBuffer.clear(sf::Color::Transparent);
    m_neonBuffer.draw(m_neonSprite, &extractShader);
    m_neonBuffer.display();

    //blur passs
    auto& blurShader = m_shaders.get(ShaderID::Blur);
    blurShader.setUniform("u_texture", m_neonBuffer.getTexture());
    blurShader.setUniform("u_offset", sf::Vector2f(1.f / GameConst::SmallBufferSize.x, 0.f));
    m_neonSprite.setTexture(m_neonBuffer.getTexture(), true);
    m_blurBuffer.clear(sf::Color::Transparent);
    m_blurBuffer.draw(m_neonSprite, &blurShader);
    m_blurBuffer.display();

    blurShader.setUniform("u_texture", m_blurBuffer.getTexture());
    blurShader.setUniform("u_offset", sf::Vector2f(0.f, 1.f / GameConst::SmallBufferSize.y));
    m_neonSprite.setTexture(m_blurBuffer.getTexture(), true);
    m_neonBuffer.setView(m_neonBuffer.getDefaultView());
    m_neonBuffer.clear(sf::Color::Transparent);
    m_neonBuffer.draw(m_neonSprite, &blurShader);
    m_neonBuffer.display();
}

void RenderPath::renderBasic(xy::Scene& backgroundScene, xy::Scene& gameScene)
{
    auto& shader = m_shaders.get(ShaderID::Sprite3DTextured);
    if (m_cameras.empty())
    {
        auto cam = gameScene.getActiveCamera();
        shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));

        m_backgroundBuffer.setView(m_backgroundBuffer.getDefaultView());
        m_backgroundBuffer.clear();
        m_backgroundBuffer.draw(m_backgroundSprite);
        m_backgroundBuffer.draw(backgroundScene);
        m_backgroundBuffer.display();

        m_gameSceneBuffer.setActive(true);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_gameSceneBuffer.draw(gameScene);
        m_gameSceneBuffer.display();
    }
    else
    {
        m_gameSceneBuffer.setView(m_gameSceneBuffer.getDefaultView());

        m_gameSceneBuffer.setActive(true);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_gameSceneBuffer.draw(m_backgroundSprite);

        for (auto cam : m_cameras)
        {
            shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));

            gameScene.setActiveCamera(cam);
            m_gameSceneBuffer.draw(gameScene);
        }
        m_gameSceneBuffer.display();
    }
}

void RenderPath::draw(sf::RenderTarget& rt, sf::RenderStates) const
{
    rt.draw(m_gameSceneSprite, m_blendShader);
}