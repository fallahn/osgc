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
#include "ActorIDs.hpp"
#include "NetActor.hpp"

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
        gs.actorCount = sd.playerCount; //TODO add other actors such as roids
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
        case PacketID::ClientInput:
            updatePlayerInput(m_players[evt.peer.getID()].entity, packet.as<InputUpdate>());
            break;
        }
    }
}

void RaceState::netUpdate(float)
{   
    for (const auto& p : m_players)
    {
#ifdef XY_DEBUG
        //send server side position (debug, remove this)
        m_sharedData.netHost.sendPacket(p.second.peer, PacketID::DebugPosition, p.second.entity.getComponent<xy::Transform>().getPosition(), xy::NetFlag::Unreliable);
#endif //XY_DEBUG

        //send reconciliation data to each player
        const auto& tx = p.second.entity.getComponent<xy::Transform>();
        const auto& vehicle = p.second.entity.getComponent<Vehicle>();
        ClientUpdate cu;
        cu.x = tx.getPosition().x;
        cu.y = tx.getPosition().y;
        cu.rotation = tx.getRotation();
        cu.velX = vehicle.velocity.x;
        cu.velY = vehicle.velocity.y;
        cu.velRot = vehicle.anglularVelocity;
        cu.clientTimestamp = vehicle.history[vehicle.lastUpdatedInput].timestamp;
        m_sharedData.netHost.sendPacket(p.second.peer, PacketID::ClientUpdate, cu, xy::NetFlag::Unreliable);
    }

    //TODO broadcast data for each actor

    //TODO send stats updates such as scores
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
    m_scene.addSystem<NetActorSystem>(m_messageBus);
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
        auto peerID = m_sharedData.peerIDs[i];
        auto result = std::find_if(m_sharedData.clients.begin(), m_sharedData.clients.end(),
            [peerID](const xy::NetPeer & peer) {return peer.getID() == peerID; });

        if (result != m_sharedData.clients.end())
        {
            //create vehicle entity
            auto entity = m_scene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
            entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(m_sharedData.vehicleIDs[i]);

            switch (entity.getComponent<Vehicle>().type)
            {
            default:
            case Vehicle::Car:
                entity.getComponent<Vehicle>().settings = Definition::car;
                entity.addComponent<NetActor>().actorID = ActorID::Car;
                //TODO collision bounds
                break;
            case Vehicle::Bike:
                entity.getComponent<Vehicle>().settings = Definition::bike;
                entity.addComponent<NetActor>().actorID = ActorID::Bike;
                break;
            case Vehicle::Ship:
                entity.getComponent<Vehicle>().settings = Definition::ship;
                entity.addComponent<NetActor>().actorID = ActorID::Ship;
                break;
            }
            entity.getComponent<NetActor>().colourID = i;
            entity.getComponent<NetActor>().serverID = entity.getIndex();

            //map entity to peerID
            m_players[peerID].entity = entity;
            m_players[peerID].peer = *result;
        }
        else
        {
            //client disconnected somewhere.
            //TODO handle this
        }
    }

    //TODO create asteroids

    return true;
}

void RaceState::sendPlayerData(const xy::NetPeer& peer)
{
    //actual player entity for this peer
    const auto& player = m_players[peer.getID()];

    VehicleData data;
    data.vehicleType = player.entity.getComponent<Vehicle>().type;
    data.x = player.entity.getComponent<xy::Transform>().getPosition().x;
    data.y = player.entity.getComponent<xy::Transform>().getPosition().y;
    data.colourID = static_cast<std::uint8_t>(player.entity.getComponent<NetActor>().colourID);

    m_sharedData.netHost.sendPacket(peer, PacketID::VehicleData, data, xy::NetFlag::Reliable);


    //all other actors are 'dumb' representations
    const auto& actors = m_scene.getSystem<NetActorSystem>().getActors();
    for (auto actor : actors)
    {
        if (actor != player.entity)
        {
            auto netActor = actor.getComponent<NetActor>();
            const auto& tx = actor.getComponent<xy::Transform>();

            ActorData data;
            data.actorID = static_cast<std::int16_t>(netActor.actorID);
            data.serverID = static_cast<std::int16_t>(netActor.serverID);
            data.colourID = static_cast<std::uint8_t>(netActor.colourID);
            data.x = tx.getPosition().x;
            data.y = tx.getPosition().y;
            data.rotation = tx.getRotation();

            m_sharedData.netHost.sendPacket(peer, PacketID::ActorData, data, xy::NetFlag::Reliable);
        }
    }
}

void RaceState::updatePlayerInput(xy::Entity entity, const InputUpdate& iu)
{
    auto& vehicle = entity.getComponent<Vehicle>();

    Input input;
    input.flags = iu.inputFlags;
    input.timestamp = iu.timestamp;
    input.multiplier = iu.acceleration;

    //update player input history
    vehicle.history[vehicle.currentInput] = input;
    vehicle.currentInput = (vehicle.currentInput + 1) % vehicle.history.size();
}