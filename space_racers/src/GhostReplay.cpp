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

#include "GhostReplay.hpp"
#include "MessageIDs.hpp"
#include "GameConsts.hpp"
#include "VehicleSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

#include <string>

namespace
{
    const std::string GhostFragment =
        R"(
            #version 120

            uniform sampler2D u_texture;

            void main()
            {
                vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy) * gl_Color;
                gl_FragColor = colour;
            }
        )";
}

GhostReplay::GhostReplay(xy::Scene& scene, xy::ResourceHandler& resources,
                            std::array<xy::Sprite, SpriteID::Game::Count>& sprites)
    : m_scene           (scene),
    m_resources         (resources),
    m_sprites           (sprites),
    m_recordedPoints    (MaxPoints),
    m_recordingIndex    (0),
    m_playbackPoints    (MaxPoints),
    m_playbackIndex     (0),
    m_enabled           (false)
{
    m_shader.loadFromMemory(GhostFragment, sf::Shader::Fragment);
}

//public
void GhostReplay::setPlayerEntity(xy::Entity e)
{
    m_playerEntity = e;
}

void GhostReplay::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        if (data.type == GameEvent::RaceStarted)
        {
            m_enabled = true;
        }
        else if (data.type == GameEvent::RaceEnded)
        {
            m_enabled = false;
        }
    }
    else if (msg.id == MessageID::VehicleMessage)
    {
        const auto& data = msg.getData<VehicleEvent>();
        switch (data.type)
        {
        default: break;
        case VehicleEvent::LapLine:
            m_playbackPoints.swap(m_recordedPoints);
            m_playbackIndex = 0;
            m_recordingIndex = 0;

            if (!m_ghostEntity.isValid())
            {
                createGhost();
            }
            break;
        }
    }
}

void GhostReplay::update()
{
    if (m_enabled)
    {
        //record position
        const auto& tx = m_playerEntity.getComponent<xy::Transform>();
        m_recordedPoints[m_recordingIndex] = { tx.getPosition(), tx.getRotation(), tx.getScale().x };
        m_recordingIndex = (m_recordingIndex + 1) % MaxPoints;

        //update any active ghost
        if (m_ghostEntity.isValid())
        {
            auto& ghostTx = m_ghostEntity.getComponent<xy::Transform>();
            const auto& point = m_playbackPoints[m_playbackIndex];

            ghostTx.setPosition(point.x, point.y);
            ghostTx.setRotation(point.rotation);
            ghostTx.setScale(point.scale, point.scale);

            m_playbackIndex = (m_playbackIndex + 1) % MaxPoints;
        }
    }
}

//private
void GhostReplay::createGhost()
{
    //TODO pick correct sprite for player vehicle
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_playbackPoints[0].x, m_playbackPoints[0].y);
    entity.getComponent<xy::Transform>().setRotation(m_playbackPoints[0].rotation);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
    entity.getComponent<xy::Drawable>().setShader(&m_shader);
    entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendMultiply);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");

    switch (m_playerEntity.getComponent<Vehicle>().type)
    {
    default:
    case Vehicle::Car:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Car];
        break;
    case Vehicle::Bike:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Bike];
        break;
    case Vehicle::Ship:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Ship];
        break;
    }

    entity.getComponent<xy::Sprite>().setColour({ 255,255,255,120 });
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width * GameConst::VehicleCentreOffset, bounds.height / 2.f);

    m_ghostEntity = entity;
}