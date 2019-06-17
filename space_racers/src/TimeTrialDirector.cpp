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

#include "TimeTrialDirector.hpp"
#include "MessageIDs.hpp"
#include "CommandIDs.hpp"
#include "SliderSystem.hpp"
#include "GameConsts.hpp"
#include "VehicleSystem.hpp"
#include "InverseRotationSystem.hpp"
#include "StateIDs.hpp"
#include "Util.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <xyginext/core/FileSystem.hpp>

#include <cstring>
#include <fstream>

namespace
{
    const std::size_t MaxLapTimes = 5;
}

TimeTrialDirector::TimeTrialDirector(ResourceCollection rc, const std::string& mapName, std::int32_t vt, SharedData& sd)
    :  m_updateDisplay (false),
    m_fastestLap    (99.f*60.f),
    m_resources     (rc),
    m_mapName       (mapName),
    m_vehicleType   (vt),
    m_sharedData    (sd),
    m_recordedPoints(MaxPoints),
    m_recordingIndex(0),
    m_playbackPoints(MaxPoints),
    m_playbackIndex (0),
    m_ghostEnabled  (false)
{
    m_mapName = m_mapName.substr(0, m_mapName.find(".tmx"));
    sd.lapTimes.clear();
}

//public
void TimeTrialDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        if (data.type == GameEvent::RaceStarted)
        {
            m_lapClock.restart();
            m_updateDisplay = true;
            m_ghostEnabled = true;

            loadGhost();
        }
        else if (data.type == GameEvent::RaceEnded)
        {
            m_updateDisplay = false;
            m_ghostEnabled = false;
        }
        /*else if (data.type == GameEvent::NewBestTime)
        {
            m_playbackPoints.swap(m_recordedPoints);
        }*/
    }
    else if (msg.id == MessageID::VehicleMessage)
    {
        const auto& data = msg.getData<VehicleEvent>();
        if (data.type == VehicleEvent::LapLine)
        {
            auto lapTime = m_lapClock.restart().asSeconds();
            if (lapTime < m_fastestLap)
            {
                m_fastestLap = lapTime;

                xy::Command cmd;
                cmd.targetFlags = CommandID::UI::BestTimeText;
                cmd.action = [lapTime](xy::Entity entity, float)
                {
                    entity.getComponent<xy::Text>().setString(formatTimeString(lapTime));
                    entity.getComponent<xy::Transform>().setPosition(GameConst::LapTimePosition);
                    entity.getComponent<Slider>().target = GameConst::BestTimePosition;
                    entity.getComponent<Slider>().active = true;
                };
                sendCommand(cmd);

                auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
                msg->type = GameEvent::NewBestTime;

                m_playbackPoints.swap(m_recordedPoints);
                saveGhost();
            }

            //update ghost
            m_playbackIndex = 0;
            m_recordingIndex = 0;

            if (!m_ghostEntity.isValid())
            {
                createGhost();
            }

            //update scoreboard
            if (m_sharedData.lapTimes.size() < MaxLapTimes)
            {
                m_sharedData.lapTimes.push_back(lapTime);
                std::sort(m_sharedData.lapTimes.begin(), m_sharedData.lapTimes.end());
            }
            else if (lapTime < m_sharedData.lapTimes.back())
            {
                m_sharedData.lapTimes.back() = lapTime;
                std::sort(m_sharedData.lapTimes.begin(), m_sharedData.lapTimes.end());
            }
            m_sharedData.trackRecord = m_fastestLap;
            updateScoreboard();
        }
        else if (data.type == VehicleEvent::Respawned
            && !m_playerEntity.isValid())
        {
            m_playerEntity = data.entity;
        }
    }
}

void TimeTrialDirector::process(float)
{
    if (m_updateDisplay)
    {
        //send command to display
        float currTime = m_lapClock.getElapsedTime().asSeconds();
        xy::Command cmd;
        cmd.targetFlags = CommandID::UI::TimeText;
        cmd.action = [currTime](xy::Entity entity, float)
        {
            entity.getComponent<xy::Text>().setString(formatTimeString(currTime));
        };
        sendCommand(cmd);
    }

    //check the time to see if the player idled and quit if more than 2 minutes
    if (m_lapClock.getElapsedTime().asSeconds() > 120.f)
    {
        auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
        msg->type = GameEvent::TimedOut;
    }

    //update the ghost data
    if (m_ghostEnabled)
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
void TimeTrialDirector::createGhost()
{
    auto entity = m_resources.gameScene->createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_playbackPoints[0].x, m_playbackPoints[0].y);
    entity.getComponent<xy::Transform>().setRotation(m_playbackPoints[0].rotation);
    entity.addComponent<InverseRotation>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth + 1);
    entity.getComponent<xy::Drawable>().setShader(&m_resources.shaders->get(ShaderID::Ghost));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseMap");
    entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.resources->get<sf::Texture>(m_resources.textureIDs->at(TextureID::Game::VehicleNormal)));
    entity.getComponent<xy::Drawable>().bindUniform("u_specularMap", m_resources.resources->get<sf::Texture>(m_resources.textureIDs->at(TextureID::Game::VehicleSpecular)));
    entity.getComponent<xy::Drawable>().bindUniform("u_lightRotationMatrix", entity.getComponent<InverseRotation>().matrix.getMatrix());
    
    switch (m_vehicleType)
    {
    default:
    case Vehicle::Car:
        entity.addComponent<xy::Sprite>() = m_resources.sprites->at(SpriteID::Game::Car);
        break;
    case Vehicle::Bike:
        entity.addComponent<xy::Sprite>() = m_resources.sprites->at(SpriteID::Game::Bike);
        break;
    case Vehicle::Ship:
        entity.addComponent<xy::Sprite>() = m_resources.sprites->at(SpriteID::Game::Ship);
        break;
    }

    entity.getComponent<xy::Sprite>().setColour({ 255,255,255,120 });
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width * GameConst::VehicleCentreOffset, bounds.height / 2.f);

    m_ghostEntity = entity;
}

void TimeTrialDirector::loadGhost()
{
    auto path = getGhostPath();
    if (xy::FileSystem::fileExists(path))
    {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open() && file.good())
        {
            auto expectedSize = MaxPoints * sizeof(Point);
            expectedSize += sizeof(float);

            //cos I find myself doing it a lot
            LOG("MAKE THIS A XYGINE UTIL", xy::Logger::Type::Info);

            file.seekg(0, file.end);
            auto fileSize = file.tellg();
            if (fileSize != expectedSize)
            {
                file.close();
                return;
            }
            file.seekg(file.beg);

            std::vector<char> buffer(fileSize);
            file.read(buffer.data(), fileSize);
            file.close();

            std::memcpy(&m_fastestLap, buffer.data(), sizeof(float));
            std::memcpy(m_playbackPoints.data(), buffer.data() + sizeof(float), MaxPoints * sizeof(Point));

            createGhost();

            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::BestTimeText;
            cmd.action = [&](xy::Entity entity, float)
            {
                entity.getComponent<xy::Text>().setString(formatTimeString(m_fastestLap));
            };
            sendCommand(cmd);
        }
    }
    else
    {
        xy::FileSystem::createDirectory(xy::FileSystem::getConfigDirectory(GameConst::AppName) + m_mapName);
    }
}

void TimeTrialDirector::saveGhost()
{
    auto path = getGhostPath();

    auto fileSize = MaxPoints * sizeof(Point);
    fileSize += sizeof(float);

    std::vector<char> buffer(fileSize);
    std::memcpy(buffer.data(), &m_fastestLap, sizeof(float));
    std::memcpy(buffer.data() + sizeof(float), m_playbackPoints.data(), sizeof(Point) * MaxPoints);

    std::ofstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        file.write(buffer.data(), buffer.size());
    }
    else
    {
        xy::Logger::log("Failed writing ghost to " + path, xy::Logger::Type::Error);
    }
    file.close();
}

std::string TimeTrialDirector::getGhostPath() const
{
    auto path = xy::FileSystem::getConfigDirectory(GameConst::AppName);
    path += m_mapName;
    path += "/" + std::to_string(m_vehicleType) + ".gst";
    return path;
}

void TimeTrialDirector::updateScoreboard()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::UI::TopTimesText;
    cmd.action = [&](xy::Entity e, float)
    {
        std::string str("Lap Times\n");
        for (auto i = 0u; i < m_sharedData.lapTimes.size(); ++i)
        {
            str += std::to_string(i + 1) + ". " + formatTimeString(m_sharedData.lapTimes[i]) + "\n";
        }
        e.getComponent<xy::Text>().setString(str);
    };
    sendCommand(cmd);
}