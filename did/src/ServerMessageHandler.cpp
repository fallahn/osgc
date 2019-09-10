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

#include "SharedStateData.hpp"
#include "MessageIDs.hpp"

#include <xyginext/core/App.hpp>

void serverMessageHandler(SharedData& sharedData, const xy::Message& msg)
{
    if (msg.id == MessageID::SystemMessage)
    {
        const auto& data = msg.getData<SystemEvent>();
        if (data.action == SystemEvent::RequestStartServer
            && !sharedData.gameServer->running())
        {
            sharedData.gameServer->start();

            sf::Clock clock;
            while (clock.getElapsedTime().asSeconds() < 1.f) {} //TODO change this to while(notRunning) ? 

            auto newMsg = xy::App::getActiveInstance()->getMessageBus().post<SystemEvent>(MessageID::SystemMessage);

            //if (sharedData.gameServer->getSteamID().IsValid())
            {
                //sharedData.serverID = sharedData.gameServer->getSteamID();
                xy::Logger::log("Created server instance", xy::Logger::Type::Info);
                newMsg->action = SystemEvent::ServerStarted;
            }
            /*else //TODO restore this when we unkludge
            {
                sharedData.serverID = {};
                sharedData.gameServer->stop();
                xy::Logger::log("Failed creating game server instance", xy::Logger::Type::Error);
                newMsg->action = SystemEvent::ServerStopped;
            }*/
        }
        else if (data.action == SystemEvent::RequestStopServer)
        {
            sharedData.gameServer->stop();
        }
    }
}