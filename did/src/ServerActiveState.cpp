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

#include "ServerActiveState.hpp"
#include "ServerSharedStateData.hpp"
#include "Server.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/network/NetData.hpp>

using namespace Server;

namespace
{
    struct TestCallback final
    {
        TestCallback(xy::Scene& s) : scene(s){}

        void operator() (xy::Entity e, float dt)
        {
            timeout -= dt;
            if (timeout < 0)
            {
                scene.destroyEntity(e);
            }
        }
        float timeout = 10.f;
        xy::Scene& scene;
    };
}

ActiveState::ActiveState(SharedStateData& sd)
    : m_sharedData  (sd)
{
    setNextState(getID());
    xy::Logger::log("Server switched to active state");
}

ActiveState::~ActiveState()
{
    //TODO do we need to tell connected clients to clear up?
    //after all if they remain connected when this state is
    //closed they ought to be changing state themselves.

    //clients disconnecting from a lobby can tidy up locally.
}

//public
void ActiveState::networkUpdate(float dt)
{

}

void ActiveState::logicUpdate(float dt)
{

}

void ActiveState::handlePacket(const xy::NetEvent& evt)
{
    switch (evt.packet.getID())
    {
    default: break;
    case PacketID::StartGame:
        //start game on request
        setNextState(Server::StateID::Running);
        m_sharedData.gameServer->broadcastData(PacketID::LaunchGame, std::uint8_t(0), xy::NetFlag::Reliable);
        break;
    case PacketID::SetSeed:
    {
        m_sharedData.seedData = evt.packet.as<Server::SeedData>();
        std::hash<std::string> hash;
        m_sharedData.seedData.hash = hash(std::string(m_sharedData.seedData.str));
    }
        m_sharedData.gameServer->broadcastData(PacketID::CurrentSeed, m_sharedData.seedData, xy::NetFlag::Reliable);
        break;
    case PacketID::RequestSeed:
        m_sharedData.gameServer->broadcastData(PacketID::CurrentSeed, m_sharedData.seedData, xy::NetFlag::Reliable);
        break;
    }
}

void ActiveState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::ServerMessage)
    {
        const auto& data = msg.getData<ServerEvent>();
        if (data.type == ServerEvent::ClientConnected)
        {
            m_sharedData.gameServer->broadcastData(PacketID::CurrentSeed, m_sharedData.seedData, xy::NetFlag::Reliable);
        }
    }
}

//private