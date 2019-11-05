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

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/core/App.hpp>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>

#include <algorithm>
#include <cmath>

namespace
{
    const float ViewDistance = GameConst::RoomWidth * 3.f;
}

Render3DSystem::Render3DSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(Render3DSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<xy::Transform>();
}

//public
void Render3DSystem::process(float)
{
    m_drawList.clear();
    auto camPos = m_camera.getComponent<xy::Transform>().getPosition();

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& tx = entity.getComponent<xy::Transform>();
        auto pos = tx.getPosition();

        //TODO take into account camera rotation....

        //if we're behind the camera cull immediately
        if (pos.y > camPos.y || pos.y < (camPos.y - ViewDistance))
        {
            continue;
        }

        //point line test. if either of the top corners of the drawable
        //bounds lie between the frustum lines it must be visible
        const auto& drawable = entity.getComponent<xy::Drawable>();
        auto worldBounds = tx.getWorldTransform().transformRect(drawable.getLocalBounds());

        sf::Vector2f posA(worldBounds.left, worldBounds.top + worldBounds.height);
        posA -= camPos;

        float resultALeft = lineSide(m_frustum.left, posA);
        float resultARight = lineSide(m_frustum.right, posA);
        if (resultALeft < 0 && resultARight > 0)
        {
            m_drawList.push_back(entity);
            continue;
        }

        sf::Vector2f posB(worldBounds.left + worldBounds.width, worldBounds.top + worldBounds.height);
        posB -= camPos;

        float resultBLeft = lineSide(m_frustum.left, posB);
        float resultBRight = lineSide(m_frustum.right, posB);
        if (resultBLeft < 0 && resultBRight > 0)
        {
            m_drawList.push_back(entity);
            continue;
        }

        //else if both the points straddle the frustum then the drawable passes right across it
        if (resultALeft > 0 && resultBRight < 0)
        {
            m_drawList.push_back(entity);
            continue;
        }
    }

    //depth sort drawlist
    std::sort(m_drawList.begin(), m_drawList.end(),
        [](const xy::Entity& entA, const xy::Entity& entB)
    {
        return entA.getComponent<xy::Drawable>().getDepth() < entB.getComponent<xy::Drawable>().getDepth();
    });
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
    for (auto entity : m_drawList)
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