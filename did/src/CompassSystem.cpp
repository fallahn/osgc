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

#include "CompassSystem.hpp"
#include "GlobalConsts.hpp"
#include "MessageIDs.hpp"
#include "Actor.hpp"
#include "Camera3D.hpp"
#include "PlayerSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

#include <SFML/Graphics/Transform.hpp>

namespace
{
#include "GroundShaders.inl"

    const float ViewRadius = 180.f;
    const float ViewRadiusSqr = ViewRadius * ViewRadius;

    const float NeedleWidth = 64.f;
    const float NeedleHeight = 16.f;

    const float TextureHalfWidth = NeedleWidth / 2.f;
    const float TextureHalfHeight = NeedleHeight / 2.f;

    const float CompassWidth = 64.f;
    const float CompassHeight = 64.f;
}

CompassSystem::CompassSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(CompassSystem))
{
    requireComponent<Compass>();
    requireComponent<xy::Transform>();
    requireComponent<xy::Drawable>();

    m_shader.loadFromMemory(GroundVert, CompassFrag);
}

//public
void CompassSystem::handleMessage(const xy::Message& msg) 
{
    if (msg.id == MessageID::MiniMapUpdate)
    {
        const auto& data = msg.getData<MiniMapEvent>();
        m_playerPoints[data.actorID - Actor::ID::PlayerOne] = data.position;
    }
}

void CompassSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto parent = entity.getComponent<Compass>().parent;
        auto currentId = parent.getComponent<Player>().playerNumber;
        auto& drawable = entity.getComponent<xy::Drawable>();
        auto position = parent.getComponent<xy::Transform>().getWorldPosition();
        position.x += 16.f;

        std::size_t currVert = 0;
        auto& verts = drawable.getVertices();

        //compass background
        verts[currVert].position = position - (sf::Vector2f(CompassWidth, CompassHeight) / 4.f);
        verts[currVert].texCoords = { 0.f, NeedleHeight };
        currVert++;

        verts[currVert].position = position + sf::Vector2f(CompassWidth / 4.f, -CompassHeight / 4.f);
        verts[currVert].texCoords = { CompassWidth, NeedleHeight };
        currVert++;

        verts[currVert].position = position + (sf::Vector2f(CompassWidth, CompassHeight) / 4.f);
        verts[currVert].texCoords = { CompassWidth, CompassHeight + NeedleHeight };
        currVert++;

        verts[currVert].position = position + sf::Vector2f(-CompassWidth / 4.f, CompassHeight / 4.f);
        verts[currVert].texCoords = { 0.f, CompassHeight + NeedleHeight };
        currVert++;

        sf::Uint8 compassAlpha = 0;

        for (auto i = 0u; i < m_playerPoints.size(); ++i)
        {
            if (i != currentId)
            {
                auto direction = m_playerPoints[i] - position;
                auto rotation = xy::Util::Vector::rotation(direction);
                auto colour = Global::PlayerColours[i];
                auto len2 = xy::Util::Vector::lengthSquared(direction);
                if (len2 > ViewRadiusSqr)
                {
                    float diff = len2 - ViewRadiusSqr;
                    float alpha = 1.f - xy::Util::Math::clamp(diff / 6000.f, 0.f, 1.f);
                    colour.a = static_cast<sf::Uint8>(255.f * alpha);
                }

                //use the least transparent value for the compass background
                if (colour.a > compassAlpha)
                {
                    compassAlpha = colour.a;
                }

                sf::Transform xForm;
                xForm.rotate(rotation);
                xForm.scale(0.5f, 0.5f);

                verts[currVert].position = xForm.transformPoint({ -TextureHalfWidth, -TextureHalfHeight }) + position;
                verts[currVert].color = colour;
                currVert++;

                verts[currVert].texCoords = { NeedleWidth, 0.f };
                verts[currVert].position = xForm.transformPoint({ TextureHalfWidth, -TextureHalfHeight }) + position;
                verts[currVert].color = colour;
                currVert++;

                verts[currVert].texCoords = { NeedleWidth, NeedleHeight };
                verts[currVert].position = xForm.transformPoint({ TextureHalfWidth, TextureHalfHeight }) + position;
                verts[currVert].color = colour;
                currVert++;

                verts[currVert].texCoords = { 0.f, NeedleHeight };
                verts[currVert].position = xForm.transformPoint({ -TextureHalfWidth, TextureHalfHeight }) + position;
                verts[currVert].color = colour;
                currVert++;
            }
        }

        for (auto i = 0; i < 4; ++i)
        {
            verts[i].color.a = compassAlpha / 4;
        }

        drawable.updateLocalBounds();
    }

    //update from camera settings
    const auto camEnt = getScene()->getActiveCamera();
    const auto& camera = camEnt.getComponent<Camera3D>();

    auto viewProj = sf::Glsl::Mat4(&camera.viewProjectionMatrix[0][0]);
    m_shader.setUniform("u_viewProjectionMatrix", viewProj);
}

void CompassSystem::onEntityAdded(xy::Entity entity)
{
    entity.getComponent<xy::Drawable>().getVertices().resize(4 * 4); //background and 3 pointers
    entity.getComponent<xy::Drawable>().setPrimitiveType(sf::Quads);
    entity.getComponent<xy::Drawable>().setShader(&m_shader);
}