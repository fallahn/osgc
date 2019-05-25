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
#include "MessageIDs.hpp"

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
    m_nextState     (StateID::Race),
    m_mapParser     (m_scene),
    m_state         (Prepping),
    m_unpauseState  (Countdown)
{
    initScene();

    //load map and create players based on shared data
    if (loadMap() && createPlayers())
    {
        GameStart gs;
        gs.gameMode = GameMode::Race;
        gs.actorCount = sd.lobbyData.playerCount;
        gs.mapIndex = sd.lobbyData.mapIndex;

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
    if (msg.id == MessageID::VehicleMessage)
    {
        const auto& data = msg.getData<VehicleEvent>();
        if (data.type == VehicleEvent::RequestRespawn)
        {
            auto entity = data.entity;

            auto& vehicle = entity.getComponent<Vehicle>();
            auto& tx = entity.getComponent<xy::Transform>();

            vehicle.velocity = {};
            vehicle.anglularVelocity = {};
            vehicle.stateFlags = (1 << Vehicle::Normal);
            vehicle.invincibleTime = GameConst::InvincibleTime;

            tx.setPosition(vehicle.waypointPosition);
            tx.setRotation(vehicle.waypointRotation);
            tx.setScale(1.f, 1.f);

            m_sharedData.netHost.broadcastPacket(PacketID::VehicleSpawned, entity.getIndex(), xy::NetFlag::Reliable);
        }
        else if (data.type == VehicleEvent::Exploded)
        {
            m_sharedData.netHost.broadcastPacket(PacketID::VehicleExploded, data.entity.getIndex(), xy::NetFlag::Reliable);
        }
        else if (data.type == VehicleEvent::Fell)
        {
            m_sharedData.netHost.broadcastPacket(PacketID::VehicleFell, data.entity.getIndex(), xy::NetFlag::Reliable);
        }
        else if (data.type == VehicleEvent::LapLine)
        {
            //TODO broadcast this to clients
            for (auto& p : m_players)
            {
                if (p.second.entity == data.entity)
                {
                    p.second.lapCount--;
                    if (p.second.lapCount == 0)
                    {
                        p.second.entity.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Disabled);
                        p.second.ready = false;
                    }
                    break;
                }
            }
        }
    }

    m_scene.forwardMessage(msg);
}

void RaceState::handleNetEvent(const xy::NetEvent& evt)
{
    if (evt.type == xy::NetEvent::PacketReceived)
    {
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
            m_players[evt.peer.getID()].ready = true;
            break;
        }
    }
    else if (evt.type == xy::NetEvent::ClientDisconnect)
    {
        //tidy up client resources and relay to remaining clients
        auto result = std::find_if(m_sharedData.clients.begin(), m_sharedData.clients.end(),
            [&evt](const std::pair<xy::NetPeer, std::uint64_t>& pair)
            {
                return pair.first == evt.peer;
            });

        if (result != m_sharedData.clients.end())
        {
            auto id = result->second;
            auto entity = m_players[id].entity;

            if (entity.isValid())
            {
                m_scene.destroyEntity(entity);

                //send ent ID to clients for removal
                PlayerIdent ident;
                ident.peerID = id;
                ident.serverID = entity.getIndex();
                m_sharedData.netHost.broadcastPacket(PacketID::ClientLeftRace, ident, xy::NetFlag::Reliable);
            }

            m_players.erase(id);

            if (m_players.empty())
            {
                m_nextState = StateID::Lobby;
            }
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
        cu.collisionFlags = vehicle.collisionFlags;
        cu.stateFlags = vehicle.stateFlags;
        m_sharedData.netHost.sendPacket(p.second.peer, PacketID::ClientUpdate, cu, xy::NetFlag::Unreliable);

        //send actor updates only for visible
        /*sf::FloatRect queryBounds = { tx.getPosition() - (xy::DefaultSceneSize / 2.f), xy::DefaultSceneSize };
        auto actors = m_scene.getSystem<xy::DynamicTreeSystem>().query(queryBounds, CollisionFlags::Vehicle | CollisionFlags::Asteroid);
        for (auto actor : actors)
        {
            const auto& tx = actor.getComponent<xy::Transform>();
            auto velocity = actor.getComponent<NetActor>().velocity;

            ActorUpdate au;
            au.serverID = static_cast<std::uint16_t>(actor.getIndex());
            au.rotation = tx.getRotation();
            au.x = tx.getPosition().x;
            au.y = tx.getPosition().y;
            au.velX = velocity.x;
            au.velY = velocity.y;
            au.timestamp = getServerTime();

            m_sharedData.netHost.sendPacket(p.second.peer, PacketID::ActorUpdate, au, xy::NetFlag::Unreliable);
        }*/
    }

    //broadcast data for each actor
    const auto& actors = m_scene.getSystem<NetActorSystem>().getActors();
    for (auto actor : actors)
    {
        const auto& tx = actor.getComponent<xy::Transform>();
        auto velocity = actor.getComponent<NetActor>().velocity;

        ActorUpdate au;
        au.serverID = static_cast<std::uint16_t>(actor.getIndex());
        au.rotation = tx.getRotation();
        au.x = tx.getPosition().x;
        au.y = tx.getPosition().y;
        au.velX = velocity.x;
        au.velY = velocity.y;
        au.timestamp = getServerTime();

        m_sharedData.netHost.broadcastPacket(PacketID::ActorUpdate, au, xy::NetFlag::Unreliable);
    }

    //TODO send stats updates such as scores
}

std::int32_t RaceState::logicUpdate(float dt)
{
    switch (m_state)
    {
    default: break;
    case Prepping:
        //waiting for clients to all ready up
    {
        bool ready = true;
        for (const auto& p : m_players)
        {
            ready = p.second.ready;
            if (!ready)
            {
                break;
            }
        }

        if (ready)
        {
            m_state = Pause;
            m_pauseTime = sf::seconds(2.f);
            m_pauseClock.restart();
        }
    }
        break;
    case Countdown:
        //counting to start
    {
        static const sf::Time countInTime = sf::seconds(3.f);
        if (m_countDownTimer.getElapsedTime() > countInTime)
        {
            for (auto& p : m_players)
            {
                p.second.entity.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Normal);
            }

            m_sharedData.netHost.broadcastPacket(PacketID::RaceStarted, std::uint8_t(0), xy::NetFlag::Reliable);
            m_state = Racing;
        }
    }
        break;
    case Racing:
        //game is a GO!
        //check player ready state (set to false once crossed the finish line)
        //if all false the game is over!
    {
        std::uint32_t readyCount = 0;
        for (const auto& p : m_players)
        {
            if (!p.second.ready)
            {
                readyCount++;
            }
        }

        if (readyCount == m_players.size())
        {
            m_state = RaceOver;
            m_sharedData.netHost.broadcastPacket(PacketID::RaceFinished, std::uint8_t(0), xy::NetFlag::Reliable);
        }
        else if (readyCount == m_players.size() - 1)
        {
            //we're in timeout mode
            //TODO skip to game over on timeout
            
        }
    }
        break;
    case RaceOver:
        //I forget what I was going to do here. let's just get the lobby running again
        m_nextState = sv::StateID::Lobby;
        
        break;
    case Pause:
        if (m_pauseClock.getElapsedTime() > m_pauseTime)
        {
            m_state = m_unpauseState;
            m_countDownTimer.restart();
            m_sharedData.netHost.broadcastPacket(PacketID::CountdownStarted, std::uint8_t(0), xy::NetFlag::Reliable);
        }
        break;
    }

    m_scene.update(dt);
    return m_nextState;
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
    bounds.left -= 1000.f;
    bounds.top -= 1000.f;
    bounds.width += 2000.f;
    bounds.height += 2000.f;
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

        entity.addComponent<CollisionObject>().type = CollisionObject::Type::Roid;
        entity.getComponent<CollisionObject>().applyVertices(createCollisionCircle(radius * 0.9f, { radius, radius }));

        m_sharedData.lobbyData.playerCount++; //this is so the client knows the total actor count and can notify when all have been spawned
    }

    return true;
}

bool RaceState::createPlayers()
{
    auto i = 0;
    for(const auto& playerInfo : m_sharedData.playerInfo)
    {
        auto peerID = playerInfo.first;
        auto result = std::find_if(m_sharedData.clients.begin(), m_sharedData.clients.end(),
            [peerID](const std::pair<xy::NetPeer, std::uint64_t>& peer) {return peer.second == peerID; });

        if (result != m_sharedData.clients.end())
        {
            //create vehicle entity
            auto entity = m_scene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(m_mapParser.getStartPosition());
            entity.getComponent<xy::Transform>().move(GameConst::SpawnPositions[i]); //TODO rotate the offset by spawn pos rot
            entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(playerInfo.second.vehicle);
            entity.getComponent<Vehicle>().waypointCount = m_mapParser.getWaypointCount();

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

            entity.getComponent<NetActor>().colourID = i++;
            entity.getComponent<NetActor>().serverID = entity.getIndex();
            
            //map entity to peerID
            m_players[playerInfo.first].entity = entity;
            m_players[playerInfo.first].peer = result->first;
            m_players[playerInfo.first].ready = false;
            m_players[playerInfo.first].lapCount = m_sharedData.lobbyData.lapCount + 1; //add one because we get the initial start line counted
        }
        else
        {
            //client disconnected somewhere.
            //TODO check this is handled correctly by net event
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
            data.timestamp = getServerTime();

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