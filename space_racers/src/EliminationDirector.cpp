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

#include "EliminationDirector.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "VehicleSystem.hpp"
#include "CameraTarget.hpp"
#include "StateIDs.hpp"
#include "TrailSystem.hpp"
#include "WayPoint.hpp"

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/CommandSystem.hpp>

namespace
{
    sf::Time CelebrationTime = sf::seconds(3.f);
}

EliminationDirector::EliminationDirector(SharedData& sd, xy::Scene& uiScene)
    : m_sharedData  (sd),
    m_uiScene       (uiScene),
    m_state         (Readying),
    m_suddenDeath   (false)
{

}

//public
void EliminationDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        if (data.type == GameEvent::SuddenDeath)
        {
            m_suddenDeath = true;
        }
    }
}

void EliminationDirector::process(float)
{
    switch (m_state)
    {
    default: break;
    case Readying:
        if (m_stateTimer.getElapsedTime() > sf::seconds(3.f))
        {
            m_state = Counting;
            m_stateTimer.restart();

            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::StartLights;
            cmd.action = [](xy::Entity e, float dt)
            {
                e.getComponent<xy::SpriteAnimation>().play(0);
                
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = CommandID::Game::StartLights;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::AudioEmitter>().play();
            };
            sendCommand(cmd);
        }
        break;
    case Counting:
        if (m_stateTimer.getElapsedTime() > sf::seconds(4.f))
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::StartLights;
            cmd.action = [](xy::Entity e, float dt)
            {
                e.getComponent<xy::Callback>().active = true;
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
            msg->type = GameEvent::RaceStarted;

            cmd.targetFlags = CommandID::Game::LapLine;
            cmd.action = [](xy::Entity e, float)
            {
                //a bit kludgy but it makes the lights turn green! :P
                auto& verts = e.getComponent<xy::Drawable>().getVertices();
                for (auto i = 8u; i < 16u; ++i)
                {
                    verts[i].texCoords.x -= 192.f;
                }
            };
            sendCommand(cmd);

            m_state = Racing;
        }
        break;
    case Racing:
    {
        //sort the play positions and tell the camera to target leader
        std::sort(m_playerEntities.begin(), m_playerEntities.end(),
            [](xy::Entity a, xy::Entity b)
            {
                return a.getComponent<Vehicle>().totalDistance + a.getComponent<Vehicle>().lapDistance >
                    b.getComponent<Vehicle>().totalDistance + b.getComponent<Vehicle>().lapDistance;
            });
        getScene().getActiveCamera().getComponent<CameraTarget>().target = m_playerEntities[0];

        m_sharedData.localPlayers[m_playerEntities[0].getComponent<Vehicle>().colourID].position = 0;

        //check non-leading vehicles and eliminate if out of view
        auto camPosition = getScene().getActiveCamera().getComponent<xy::Transform>().getPosition();
        sf::FloatRect viewRect(camPosition - (xy::DefaultSceneSize / 2.f), xy::DefaultSceneSize);

        auto count = 0;
        for (auto i = 1; i < 4; ++i)
        {
            //update race position
            m_sharedData.localPlayers[m_playerEntities[i].getComponent<Vehicle>().colourID].position = i;

            if (m_playerEntities[i].getComponent<Vehicle>().stateFlags == (1 << Vehicle::Eliminated))
            {
                count++;
            }
            else
            {
                if (!viewRect.contains(m_playerEntities[i].getComponent<xy::Transform>().getPosition())
                    && m_playerEntities[i].getComponent<Vehicle>().invincibleTime < 0)
                {
                    eliminate(m_playerEntities[i]);
                    count++;
                }
            }
        }

        //if eliminated count == 3 switch to celebration state and award player point
        if (count == 3
            && m_playerEntities[0].getComponent<Vehicle>().stateFlags == (1 << Vehicle::Normal))
        {
            m_celebrationTimer.restart();
            m_playerEntities[0].getComponent<Vehicle>().stateFlags = (1 << Vehicle::Celebrating);
            m_playerEntities[0].getComponent<Vehicle>().accelerationMultiplier = 0.f;
            m_state = Celebrating;

            auto entity = m_playerEntities[0];
            xy::Command cmd;
            cmd.targetFlags = CommandID::Game::Trail;
            cmd.action = [entity](xy::Entity e, float)
            {
                if (e.getComponent<Trail>().parent == entity)
                {
                    e.getComponent<Trail>().parent = {};
                }
            };
            sendCommand(cmd);

            //award point to player
            //if (!m_suddenDeath)
            {
                auto playerID = m_playerEntities[0].getComponent<Vehicle>().colourID;
                m_sharedData.localPlayers[playerID].points++;

                auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
                msg->type = GameEvent::PlayerScored;
                msg->playerID = playerID;
                msg->score = m_sharedData.localPlayers[playerID].points;
                msg->position = m_playerEntities[0].getComponent<xy::Transform>().getPosition();
            }
        }
    }
    break;
    case Celebrating:
        //count celebration time
        if (m_celebrationTimer.getElapsedTime() > CelebrationTime)
        {
            //on time out check scores - if player won, end the game
            //either 4 points or if sudden death is active
            if (m_sharedData.localPlayers[m_playerEntities[0].getComponent<Vehicle>().colourID].points == 4
                || m_suddenDeath)
            {
                m_sharedData.localPlayers[m_playerEntities[0].getComponent<Vehicle>().colourID].points = 4; //forces this player to win if sudden death
                
                //sort final position by score
                auto localPlayers = m_sharedData.localPlayers;
                std::sort(localPlayers.begin(), localPlayers.end(),
                    [](const LocalPlayer& a, const LocalPlayer& b)
                    {
                        return a.points > b.points;
                    });

                for (auto& player : m_sharedData.localPlayers)
                {
                    for (auto i = 0u; i < localPlayers.size(); ++i)
                    {
                        if (player.inputBinding.controllerID == localPlayers[i].inputBinding.controllerID)
                        {
                            player.position = i;
                            break;
                        }
                    }
                }

                auto* msg = postMessage<StateEvent>(MessageID::StateMessage);
                msg->type = StateEvent::RequestPush;
                msg->id = StateID::Summary;

                auto* msg2 = postMessage<GameEvent>(MessageID::GameMessage);
                msg2->type = GameEvent::RaceEnded;

                m_state = GameOver; //prevents this being sent more than once

                break;
            }

            //else respawn all at leader's respawn point and return to race state
            const auto& leaderVehicle = m_playerEntities[0].getComponent<Vehicle>();
            auto wp = leaderVehicle.currentWaypoint;
            auto lapDistance = leaderVehicle.lapDistance;
            auto lapCount = m_sharedData.localPlayers[leaderVehicle.colourID].lapCount;

            for (auto& player : m_sharedData.localPlayers)
            {
                player.lapCount = lapCount;
            }

            for (auto e : m_playerEntities)
            {
                auto& vehicle = e.getComponent<Vehicle>();

                vehicle.currentWaypoint = wp;
                vehicle.lapDistance = lapDistance;
                vehicle.waypointDistance = wp.getComponent<WayPoint>().trackDistance;
                vehicle.totalDistance = vehicle.waypointDistance;

                auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
                msg->type = VehicleEvent::RequestRespawn;
                msg->entity = e;
            }

            auto& camTarget = getScene().getActiveCamera().getComponent<CameraTarget>();
            camTarget.lastTarget = m_playerEntities[0];
            camTarget.target = m_playerEntities[0];

            //TODO if the camera moves a long way (it shouldn't) it might eliminate players
            //as they respawn?? Tried to mitigate this by not eliminating players who have an invincibility time
            m_state = Racing;
        }
        break;
    case GameOver:
    {
        //do nothing
        for (auto entity : m_playerEntities)
        {
            entity.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Disabled);
        }
    }
        break;
    }
}

void EliminationDirector::addPlayerEntity(xy::Entity entity)
{
    m_playerEntities.push_back(entity);
}

//private
void EliminationDirector::eliminate(xy::Entity entity)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    vehicle.stateFlags = (1 << Vehicle::Eliminated);
    vehicle.accelerationMultiplier = 0.f;

    entity.getComponent<xy::Transform>().setScale(0.f, 0.f);

    auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
    msg->type = VehicleEvent::Eliminated;
    msg->entity = entity;

    xy::Command cmd;
    cmd.targetFlags = CommandID::Game::Trail;
    cmd.action = [entity](xy::Entity e, float)
    {
        if (e.getComponent<Trail>().parent == entity)
        {
            e.getComponent<Trail>().parent = {};
        }
    };
    sendCommand(cmd);
}