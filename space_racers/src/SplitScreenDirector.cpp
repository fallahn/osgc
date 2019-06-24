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

#include "SplitScreenDirector.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "VehicleSystem.hpp"
#include "StateIDs.hpp"

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/Drawable.hpp>

#include <xyginext/ecs/systems/CommandSystem.hpp>

SplitScreenDirector::SplitScreenDirector(xy::Scene& uiScene, SharedData& sd)
    : m_uiScene (uiScene),
    m_sharedData(sd),
    m_state     (Readying)
{

}

//public
void SplitScreenDirector::handleMessage(const xy::Message& msg)
{

}

void SplitScreenDirector::process(float)
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
            cmd.action = [](xy::Entity e, float dt)
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
        //sort the play positions
        std::sort(m_playerEntities.begin(), m_playerEntities.end(),
            [](xy::Entity a, xy::Entity b)
            {
                return a.getComponent<Vehicle>().totalDistance + a.getComponent<Vehicle>().lapDistance >
                    b.getComponent<Vehicle>().totalDistance + b.getComponent<Vehicle>().lapDistance;
            });

        std::size_t finishedCount = 0;
        for (auto i = 0u; i < m_playerEntities.size(); ++i)
        {
            auto id = m_playerEntities[i].getComponent<Vehicle>().colourID;
            m_sharedData.localPlayers[id].position = i;
            m_sharedData.localPlayers[id].points = (3 - i) * 10;
            if (m_sharedData.localPlayers[id].lapCount == 0)
            {
                finishedCount++;
            }
        }

        if (finishedCount == m_playerEntities.size())
        {
            auto* msg = postMessage<StateEvent>(MessageID::StateMessage);
            msg->type = StateEvent::RequestPush;
            msg->id = StateID::Summary;

            m_state = Finished;
        }
    }
        break;
    case Finished:

        break;
    }
}