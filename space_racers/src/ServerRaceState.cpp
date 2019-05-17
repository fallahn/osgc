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
#include "AsteroidSystem.hpp"
#include "CollisionObject.hpp"
#include "GameConsts.hpp"

#include <xyginext/network/NetData.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

using namespace sv;

RaceState::RaceState(SharedData& sd, xy::MessageBus& mb)
    : m_sharedData  (sd),
    m_messageBus    (mb),
    m_scene         (mb),
    m_mapParser     (m_scene)
{
    initScene();

    //load map and create players based on shared data
    if (loadMap() && createPlayers())
    {
        GameStart gs;
        gs.gameMode = GameMode::Race;
        gs.actorCount = sd.playerCount;
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
        case PacketID::ClientReady:
            std::cout << "Client sent ready status from " << evt.peer.getID() << "\n";
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

    //broadcast data for each actor
    const auto& actors = m_scene.getSystem<NetActorSystem>().getActors();
    for (auto actor : actors)
    {
        const auto& tx = actor.getComponent<xy::Transform>();

        ActorUpdate au;
        au.serverID = static_cast<std::uint16_t>(actor.getIndex());
        au.rotation = tx.getRotation();
        au.x = tx.getPosition().x;
        au.y = tx.getPosition().y;
        au.timestamp = getServerTime();

        m_sharedData.netHost.broadcastPacket(PacketID::ActorUpdate, au, xy::NetFlag::Unreliable);
    }

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
    m_scene.addSystem<AsteroidSystem>(m_messageBus);
    m_scene.addSystem<NetActorSystem>(m_messageBus);
    m_scene.addSystem<xy::DynamicTreeSystem>(m_messageBus);
}

bool RaceState::loadMap()
{
    //load collision data from map
    if (!m_mapParser.load("assets/maps/AceOfSpace.tmx"))
    {
        return false;
    }

    sf::FloatRect bounds(0.f, 0.f, 5120.f, 4096.f); //TODO get this from map data
    bounds.left -= 100.f;
    bounds.top -= 100.f;
    bounds.width += 200.f;
    bounds.height += 200.f;
    m_scene.getSystem<AsteroidSystem>().setMapSize(bounds);

    //create some roids
    auto positions = xy::Util::Random::poissonDiscDistribution(bounds, 1200, 8);
    for (auto position : positions)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position);

        sf::Vector2f velocity =
        {
            xy::Util::Random::value(-1.f, 1.f),
            xy::Util::Random::value(-1.f, 1.f)
        };
        entity.addComponent<Asteroid>().setVelocity(xy::Util::Vector::normalise(velocity) * xy::Util::Random::value(200.f, 300.f));

        sf::FloatRect aabb(0.f, 0.f, 100.f, 100.f);
        auto radius = aabb.width / 2.f;
        entity.getComponent<xy::Transform>().setOrigin(radius, radius);
        auto scale = xy::Util::Random::value(0.5f, 2.5f);
        entity.getComponent<xy::Transform>().setScale(scale, scale);
        entity.getComponent<Asteroid>().setRadius(radius * scale);

        entity.addComponent<xy::BroadphaseComponent>().setArea(aabb);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Asteroid);
        entity.addComponent<NetActor>().actorID = ActorID::Roid;
        entity.getComponent<NetActor>().serverID = entity.getIndex();

        m_sharedData.playerCount++; //this is so the client knows the total actor count and can notify when all have been spawned
    }

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
            entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize * 0.4f);
            entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(m_sharedData.vehicleIDs[i]);

            entity.addComponent<CollisionObject>().type = CollisionObject::Vehicle;
            entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);

            switch (entity.getComponent<Vehicle>().type)
            {
            default:
            case Vehicle::Car:
                entity.getComponent<Vehicle>().settings = Definition::car;
                entity.addComponent<NetActor>().actorID = ActorID::Car;
                entity.getComponent<CollisionObject>().applyVertices(GameConst::CarPoints);
                entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);                
                break;
            case Vehicle::Bike:
                entity.getComponent<Vehicle>().settings = Definition::bike;
                entity.addComponent<NetActor>().actorID = ActorID::Bike;
                entity.getComponent<CollisionObject>().applyVertices(GameConst::BikePoints);
                entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::BikeSize);
                break;
            case Vehicle::Ship:
                entity.getComponent<Vehicle>().settings = Definition::ship;
                entity.addComponent<NetActor>().actorID = ActorID::Ship;
                entity.getComponent<CollisionObject>().applyVertices(GameConst::ShipPoints);
                entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::ShipSize);
                break;
            }

            auto bounds = entity.getComponent<xy::BroadphaseComponent>().getArea();
            entity.getComponent<xy::Transform>().setOrigin(bounds.width * GameConst::VehicleCentreOffset, bounds.height / 2.f);

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
            data.scale = tx.getScale().x;

            m_sharedData.netHost.sendPacket(peer, PacketID::ActorData, data, xy::NetFlag::Reliable);
        }
    }
}

void RaceState::updatePlayerInput(xy::Entity entity, const InputUpdate& iu)
{
    auto& vehicle = entity.getComponent<Vehicle>();

    Input input; //TODO this is the same as the InputUpdate struct?
    input.flags = iu.inputFlags;
    input.timestamp = iu.timestamp;
    input.steeringMultiplier = iu.steeringMultiplier;
    input.accelerationMultiplier = iu.accelerationMultiplier;

    //update player input history
    vehicle.history[vehicle.currentInput] = input;
    vehicle.currentInput = (vehicle.currentInput + 1) % vehicle.history.size();
}