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

#include "Render3DSystem.hpp"
#include "GameConst.hpp"
#include "Camera3D.hpp"
#include "CameraTransportSystem.hpp"
#include "Sprite3D.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/core/App.hpp>
#include <xyginext/util/Vector.hpp>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>

#include <SFML/OpenGL.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    //TODO we need to get smarter with this and do some occlusion culling based on 
    //any walls that maybe in front
    const float ViewDistance = GameConst::RoomWidth * 4.8f;
    const sf::FloatRect CullBounds(-ViewDistance / 2.f, -ViewDistance * 1.1f, ViewDistance, ViewDistance);
}

Render3DSystem::Render3DSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(Render3DSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<xy::Transform>();
    requireComponent<Sprite3D>();
}

//public
void Render3DSystem::process(float)
{
    m_renderPasses[0].clear();
    m_renderPasses[1].clear();

    auto camWorldPos = m_camera.getComponent<Camera3D>().worldPosition;
    sf::Vector2f camPos(camWorldPos.x, camWorldPos.y);
    xy::App::printStat("pos", std::to_string(camWorldPos.x) + ", " + std::to_string(camWorldPos.y));
    const auto& transport = m_camera.getComponent<CameraTransport>();
    auto rotation = transport.getCurrentRotation();

    sf::Transform cullTx;
    cullTx.translate(camPos);
    cullTx.rotate(rotation);
    auto cullBounds = cullTx.transformRect(CullBounds);

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& tx = entity.getComponent<xy::Transform>();
        const auto& drawable = entity.getComponent<xy::Drawable>();
        auto worldBounds = tx.getTransform().transformRect(drawable.getLocalBounds());

        if (worldBounds.intersects(cullBounds))
        {
            auto pass = entity.getComponent<Sprite3D>().renderPass;
            m_renderPasses[pass].push_back(std::make_pair(entity, xy::Util::Vector::lengthSquared(tx.getPosition() - camPos)));
        }
    }

    //yes, drawing back to front causes overdraw, but we need this for transparency
    //TODO can we reverse this order for first pass to reduce the overdraw?
    for (auto& drawList : m_renderPasses)
    {
        std::sort(drawList.begin(), drawList.end(),
            [](const std::pair<xy::Entity, float>& a, const std::pair<xy::Entity, float>& b)
            {
                return a.second > b.second;
            });
    }
}

void Render3DSystem::setFOV(float fov)
{
    float theta = fov / 2.f;
    float x = std::atan(theta) * ViewDistance;
    
    m_frustum.left = { -x, -ViewDistance };
    m_frustum.right = { x, -ViewDistance };
}

//private
float Render3DSystem::lineSide(sf::Vector2f line, sf::Vector2f point)
{
    return (line.y * point.x) - (line.x * point.y);
}

void Render3DSystem::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    for (const auto& drawList : m_renderPasses)
    {
        for (auto [entity, dist] : drawList)
        {
            const auto& drawable = entity.getComponent<xy::Drawable>();
            const auto& tx = entity.getComponent<xy::Transform>().getWorldTransform();

            states = drawable.getStates();
            states.transform = tx;

            if (states.shader)
            {
                drawable.applyShader();
            }

            rt.draw(drawable, states);
        }
    }
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}