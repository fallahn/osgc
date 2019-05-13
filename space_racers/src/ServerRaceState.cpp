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

#include "ServerRaceState.hpp"
#include "NetConsts.hpp"
#include "ClientPackets.hpp"
#include "ServerPackets.hpp"
#include "VehicleSystem.hpp"
#include "VehicleDefs.hpp"
#include "Server.hpp"
#include "GameModes.hpp"

#include <xyginext/network/NetData.hpp>
#include <xyginext/ecs/components/Transform.hpp>

using namespace sv;

RaceState::RaceState(SharedData& sd, xy::MessageBus& mb)
    : m_sharedData  (sd),
    m_messageBus    (mb),
    m_scene         (mb)
{
    initScene();

    //load map and create players based on shared data
    if (loadMap() && createPlayers())
    {
        GameStart gs;
        gs.gameMode = GameMode::Race;
        gs.playerCount = sd.playerCount;
        gs.mapIndex = sd.mapIndex;

        sd.netHost.broadcastPacket(PacketID::GameStarted, gs, xy::NetFlag::Reliable);
    }
    //tell clients to load map if successful, else send error packet
    else
    {
        m_sharedData.netHost.broadcastPacket(PacketID::ErrorServerMap, std::uint8_t(0), xy::NetFlag::Reliable);
    }
}

//public
void RaceState::handleMessage(const xy::Message& msg)
{

    m_scene.forwardMessage(msg);
}

void RaceState::handleNetEvent(const xy::NetEvent& evt)
{
    if (evt.type == xy::NetEvent::PacketReceived)
    {
        //TODO grab player inputs and apply to vehicles
        const auto& packet = evt.packet;
        switch (packet.getID())
        {
        default: break;
        case PacketID::ClientMapLoaded:
            sendPlayerData(evt.peer);
            break;
        }
    }
}

void RaceState::netUpdate(float)
{
    //TODO broadcast data for each player
}

std::int32_t RaceState::logicUpdate(float dt)
{
    m_scene.update(dt);
    return StateID::Race;
}

//private
void RaceState::initScene()
{
    m_scene.addSystem<VehicleSystem>(m_messageBus);
}

bool RaceState::loadMap()
{
    //TODO load collision data from map

    return true;
}

bool RaceState::createPlayers()
{
    for (auto i = 0u; i < m_sharedData.playerCount; ++i)
    {
        //create vehicle entity
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.addComponent<Vehicle>();
        entity.getComponent<Vehicle>().type = static_cast<Vehicle::Type>(m_sharedData.vehicleIDs[i]);

        switch (m_sharedData.vehicleIDs[i])
        {
        default: break;
        case Vehicle::Car:
            entity.getComponent<Vehicle>().settings = Definition::car;
            //TODO collision bounds
            break;
        case Vehicle::Bike:
            entity.getComponent<Vehicle>().settings = Definition::bike;
            break;
        case Vehicle::Ship:
            entity.getComponent<Vehicle>().settings = Definition::ship;
            break;
        }

        //map entity to peerID
        m_players[m_sharedData.peerIDs[i]] = entity;
    }
    return true;
}

void RaceState::sendPlayerData(const xy::NetPeer& peer)
{
    for (const auto& player : m_players)
    {
        VehicleData data;
        data.peerID = player.first;
        data.serverEntityID = player.second.getIndex();
        data.vehicleType = player.second.getComponent<Vehicle>().type;
        data.x = player.second.getComponent<xy::Transform>().getPosition().x;
        data.y = player.second.getComponent<xy::Transform>().getPosition().y;

        m_sharedData.netHost.sendPacket(peer, PacketID::VehicleData, data, xy::NetFlag::Reliable);
    }
}