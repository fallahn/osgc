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

#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/CommandSystem.hpp>

TimeTrialDirector::TimeTrialDirector()
    : m_state(Readying)
{

}

//public
void TimeTrialDirector::handleMessage(const xy::Message&)
{

}

void TimeTrialDirector::process(float)
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
            sendCommand(cmd);

            auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
            msg->type = GameEvent::RaceStarted;
        }
        break;
    }
}