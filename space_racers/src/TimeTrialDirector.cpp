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

#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <cmath>
#include <sstream>

namespace
{
    std::string formatTimeString(float t)
    {
        float whole = 0.f;
        float remain = std::modf(t, &whole);

        int min = static_cast<int>(whole) / 60;
        int sec = static_cast<int>(whole) % 60;

        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << min << ":";
        ss << std::setw(2) << std::setfill('0') << sec << ":";
        ss << std::setw(4) << std::setfill('0') << static_cast<int>(remain * 10000.f);

        return ss.str();
    }
}

TimeTrialDirector::TimeTrialDirector()
    : m_updateDisplay(false),
    m_fastestLap(99.f*60.f)
{

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
        }
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
            }

            //TODO count laps and end time trial when done

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
}