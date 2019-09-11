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

#pragma once

#include "InputBinding.hpp"
#include "ClientInfoManager.hpp"
#include "ServerSharedStateData.hpp"
#include "Server.hpp"

#include <xyginext/network/NetClient.hpp>

#include <SFML/System/Thread.hpp>

#include <memory>

struct SharedData final
{
    InputBinding inputBinding;
    sf::String clientName;
    sf::String remoteIP;

    ClientInfoManager clientInformation;
    std::string error;
    Server::SeedData seedData;

    //these strictly shouldn't be shared
    //but it's a required fudge to enable this
    //struct to be std::any compatible
    std::shared_ptr<GameServer> gameServer;
    std::shared_ptr<xy::NetClient> netClient;
};

namespace xy
{
    class Message;
}
void serverMessageHandler(SharedData&, const xy::Message&);