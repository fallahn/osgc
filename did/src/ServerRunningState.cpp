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

#include "ServerRunningState.hpp"
#include "Packet.hpp"
#include "ServerSharedStateData.hpp"
#include "Server.hpp"
#include "PlayerSystem.hpp"
#include "CommandIDs.hpp"
#include "Actor.hpp"
#include "PacketTypes.hpp"
#include "ActorSystem.hpp"
#include "AnimationSystem.hpp"
#include "CrabSystem.hpp"
#include "ServerRandom.hpp"
#include "CollisionSystem.hpp"
#include "CollisionBounds.hpp"
#include "CarriableSystem.hpp"
#include "InventorySystem.hpp"
#include "WetPatchDirector.hpp"
#include "MessageIDs.hpp"
#include "CollectibleSystem.hpp"
#include "QuadTreeFilter.hpp"
#include "ServerWeaponDirector.hpp"
#include "SkeletonSystem.hpp"
#include "BoatSystem.hpp"
#include "ServerMessages.hpp"
#include "BotSystem.hpp"
#include "Operators.hpp"
#include "ParrotLauncherSystem.hpp"
#include "BarrelSystem.hpp"
#include "ServerStormDirector.hpp"
#include "XPSystem.hpp"
#include "BeeSystem.hpp"
#include "ConCommands.hpp"
#include "DecoySystem.hpp"
#include "TimedCarriableSystem.hpp"
#include "FlareSystem.hpp"
#include "SkullShieldSystem.hpp"
#include "InputParser.hpp"

#include <xyginext/core/Log.hpp>

#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/util/Vector.hpp>

using namespace Server;

namespace
{
    const float DayNightUpdateFrequency = 5.f;
    const std::array<sf::Vector2f, 4u> SpawnPositions = 
    {
        sf::Vector2f(7.f * Global::TileSize, 7.f * Global::TileSize),
        sf::Vector2f((Global::TileCountX * Global::TileSize) - (7.f * Global::TileSize), 7.f * Global::TileSize),
        sf::Vector2f((Global::TileCountX * Global::TileSize) - (7.f * Global::TileSize), (Global::TileCountY * Global::TileSize) - (7.f * Global::TileSize)),
        sf::Vector2f(7.f * Global::TileSize, (Global::TileCountY * Global::TileSize) - (7.f * Global::TileSize))
    };

    sf::Time RoundTime = sf::seconds(240.f);

    struct ExplosionCallback final
    {
        float lifetime = 2.1f;
        xy::Scene& scene;
        void operator() (xy::Entity entity, float dt)
        {
            lifetime -= dt;
            if (lifetime < 2.f)
            {
                entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);
            }

            if (lifetime < 0)
            {
                scene.destroyEntity(entity);
            }
        }

        explicit ExplosionCallback(xy::Scene& s) : scene(s) {}
    };
}

RunningState::RunningState(SharedStateData& sd)
    : m_sharedData      (sd),
    m_scene             (sd.gameServer->getMessageBus(), 512),
    m_dayNightUpdateTime(0.f),
    m_remainingTreasure (std::numeric_limits<std::size_t>::max())
{
    Server::rndEngine = std::mt19937();
    Server::rndEngine.seed(sd.seedData.hash);

    m_islandGenerator.generate(sd.seedData.hash);
    m_mapData = m_islandGenerator.getMapData();
    m_remainingTreasure = m_islandGenerator.getTreasureCount();
    LOG("Spawned " + std::to_string(m_remainingTreasure) + " treasures", xy::Logger::Type::Info);

    m_pathFinder.setGridSize({ static_cast<int>(Global::TileCountX), static_cast<int>(Global::TileCountY) });
    m_pathFinder.setTileSize({ Global::TileSize, Global::TileSize });
    m_pathFinder.setGridOffset({ Global::TileSize / 2.f, Global::TileSize / 2.f });
    const auto& pathData = m_islandGenerator.getPathData();
    for (auto i = 0u; i < Global::TileCount; ++i)
    {
        //only sand tiles are traversable
        if ((pathData[i] > 60 && pathData[i] != 75) || (pathData[i] == 0 || pathData[i] == 15))
        //if(m_mapData.tileData[i] > 5)
        {
            m_pathFinder.addSolidTile({ static_cast<int>(i % Global::TileCountX), static_cast<int>(i / Global::TileCountX) });
        }
    }

    createScene();

    setNextState(getID());
    xy::Logger::log("Server switched to running state");
}

//public
void RunningState::networkUpdate(float dt)
{
    //broadcast scene state - TODO will this be more efficient
    //to send only the visible actors to each player?
    auto& actors = m_scene.getSystem<ActorSystem>().getActors();
    for (auto& actor : actors)
    {
        const auto& actorComponent = actor.getComponent<Actor>();
        if (actorComponent.nonStatic)
        {
            const auto tx = actor.getComponent<xy::Transform>().getPosition(); //assume any transforms parented to another on the server are also parented client side

            ActorState state;
            state.serverID = actorComponent.serverID;
            state.actorID = actorComponent.id;
            state.direction = actorComponent.direction;
            state.position = tx;
            state.serverTime = m_sharedData.gameServer->getServerTime();

            m_sharedData.gameServer->broadcastData(PacketID::ActorUpdate, state);
        }

        auto& anim = actor.getComponent<AnimationModifier>();
        if (anim.currentAnimation != anim.nextAnimation)
        {
            AnimationState state;
            state.animation = anim.nextAnimation;
            state.serverID = actorComponent.serverID;

            //TODO check how well this works - we might need to send these reliably
            m_sharedData.gameServer->broadcastData(PacketID::AnimationUpdate, state);

            anim.currentAnimation = anim.nextAnimation;
        }
    }
    
    for (auto& client : m_playerSlots)
    {
        //client state for reconciliation
        if (client.clientID.IsValid() && client.gameEntity.isValid())
        {
            const auto tx = client.gameEntity.getComponent<xy::Transform>().getPosition();
            const auto& player = client.gameEntity.getComponent<Player>();
            const auto& actor = client.gameEntity.getComponent<Actor>();
            const auto& input = client.gameEntity.getComponent<InputComponent>();

            ClientState state;
            state.actorID = actor.id;
            state.serverID = actor.entityID;
            state.position = tx;
            state.clientTime = input.history[input.lastUpdatedInput].input.timestamp;
            state.sync = player.sync;

            m_sharedData.gameServer->sendData(PacketID::PlayerUpdate, state, client.clientID);
        }

        //and broadcast any stat changes
        if (client.sendStatsUpdate)
        {
            RoundStat stats;
            stats.id = client.gameEntity.getComponent<Actor>().id;
            stats.treasure = client.gameEntity.getComponent<Inventory>().treasure;
            stats.foesSlain = client.stats.foesSlain;
            stats.livesLost = client.stats.livesLost;
            stats.shotsFired = client.stats.shotsFired;

            m_sharedData.gameServer->broadcastData(PacketID::StatUpdate, stats, EP2PSend::k_EP2PSendReliable);

            client.sendStatsUpdate = false;
        }   
    }

    //occasionally update the day/night cycle time
    if (m_dayNightUpdateTime > DayNightUpdateFrequency)
    {
        m_dayNightUpdateTime = 0.f;

        float dayNightTime = m_dayNightClock.getElapsedTime().asSeconds();
        if (dayNightTime > Global::DayNightSeconds)
        {
            dayNightTime = 0.f;
            m_dayNightClock.restart();
        }

        float dayPosition = dayNightTime / Global::DayNightSeconds;
        //this is a kludge to offset the game start into the morning
        dayPosition = std::fmod(dayPosition + Global::DayCycleOffset, 1.f);
        m_sharedData.gameServer->broadcastData(PacketID::DayNightUpdate, dayPosition);

        auto* msg = m_sharedData.gameServer->getMessageBus().post<MapEvent>(MessageID::MapMessage);
        msg->type = MapEvent::DayNightUpdate;
        msg->value = dayPosition;

        //if we're in the last minutes of a round send the time to sync client displays
        if (m_roundTimer.started())
        {
            m_sharedData.gameServer->broadcastData(PacketID::RoundTime, m_roundTimer.getTime());
        }
    }
    m_dayNightUpdateTime += dt;
}

void RunningState::logicUpdate(float dt)
{
    m_scene.update(dt);

    if (m_roundTimer.started())
    {
        if (m_remainingTreasure == 0
            || m_roundTimer.getTime() == 0)
        {
            endGame();
        }
    }
    else
    {
        if (m_remainingTreasure == 1)
        {
            m_roundTimer.start();
            //start client timers
            m_sharedData.gameServer->broadcastData(PacketID::ServerMessage, Server::Message::OneTreasureRemaining, EP2PSend::k_EP2PSendReliable);
        }
    }
}

void RunningState::handlePacket(const Packet& packet)
{
    switch (packet.id())
    {
    default: break;
    case PacketID::RequestMap:
        m_sharedData.gameServer->sendData(PacketID::MapData, m_mapData.tileData, packet.sender, EP2PSend::k_EP2PSendReliable);
        break;
    case PacketID::RequestPlayer:
        spawnPlayer(packet.sender);
        break;
    case PacketID::ClientInput:
    {
        auto ip = packet.as<InputUpdate>();

        auto& playerIp = m_playerSlots[ip.playerNumber].gameEntity.getComponent<InputComponent>();

        //update player input history
        playerIp.history[playerIp.currentInput].input.mask = ip.input;
        playerIp.history[playerIp.currentInput].input.timestamp = ip.clientTime;
        playerIp.history[playerIp.currentInput].input.acceleration = ip.acceleration;
        playerIp.currentInput = (playerIp.currentInput + 1) % playerIp.history.size();
    }
        break;
    case PacketID::ConCommand:
        doConCommand(packet.as<ConCommand::Data>());
        break;
    }
}

void RunningState::handleMessage(const xy::Message& msg)
{
    switch (msg.id)
    {
    default: break;
    case MessageID::ActorMessage:
    {
        const auto& data = msg.getData<ActorEvent>();
        if (data.type == ActorEvent::RequestSpawn)
        {
            spawnActor(data.position, data.id);
        }
        else if (data.type == ActorEvent::Died)
        {
            //TODO use this to raise client messages / sound effects
            if (data.id == Actor::ID::Skeleton)
            {
                if (data.data >= Actor::ID::PlayerOne && data.data <= Actor::PlayerFour)
                {
                    m_playerSlots[data.data - Actor::ID::PlayerOne].stats.roundXP += XP::SkellyScore;
                }
            }
        }
    }
        break;
    case xy::Message::SceneMessage:
    {
        const auto& data = msg.getData<xy::Message::SceneEvent>();
        if (data.event == xy::Message::SceneEvent::EntityDestroyed)
        {
            SceneState state;
            state.action = SceneState::EntityRemoved;
            state.serverID = data.entityID;
            m_sharedData.gameServer->broadcastData(PacketID::SceneUpdate, state, EP2PSend::k_EP2PSendReliable);
        }
    }
        break;
    case MessageID::PlayerMessage:
    {
        const auto& data = msg.getData<PlayerEvent>();
        SceneState state;
        state.serverID = data.entity.getIndex();
    
        switch (data.action)
        {
        default: break;
        case PlayerEvent::Died:
            state.action = SceneState::PlayerDied;
            state.position = data.entity.getComponent<Player>().deathPosition;
            m_sharedData.gameServer->broadcastData(PacketID::SceneUpdate, state, EP2PSend::k_EP2PSendReliable);

            for (auto& slot : m_playerSlots)
            {
                if (slot.gameEntity == data.entity)
                {
                    slot.stats.livesLost++;
                    slot.sendStatsUpdate = true;

                    //find last actor who damaged this player
                    //and award XP if it was another player
                    auto damager = data.data;
                    if (damager >= Actor::ID::PlayerOne && damager <= Actor::ID::PlayerFour)
                    {
                        //also check if this is a bot and make sure to award correct points
                        if (slot.gameEntity.getComponent<Bot>().enabled)
                        {
                            m_playerSlots[damager - Actor::PlayerOne].stats.roundXP += XP::BotScore;
                        }
                        else
                        {
                            //find the difference in levels and calc
                            std::int64_t difference = m_playerSlots[damager - Actor::PlayerOne].stats.currentLevel -
                                slot.stats.currentLevel;
                            auto score = XP::calcPlayerScore(difference);
                            m_playerSlots[damager - Actor::PlayerOne].stats.roundXP += score;
                            std::cout << "Player Score XP: " << score << "\n";
                        }
                    }
                }
                else if (data.data == slot.gameEntity.getComponent<Actor>().id)
                {
                    //data.data is who damaged player last
                    slot.stats.foesSlain++;
                    slot.sendStatsUpdate = true;
                }
            }

            //send a message to clients
            {
                auto playerID = data.entity.getComponent<Player>().playerNumber;
                auto cause = data.entity.getComponent<Inventory>().lastDamage;
                std::int32_t messageID = -1;
                switch (playerID)
                {
                default: break;
                case 0:
                    switch (cause)
                    {
                    default: break;
                    case PlayerEvent::Drowned:
                        messageID = Server::Message::PlayerOneDrowned;
                        break;
                    case PlayerEvent::PistolHit:
                    case PlayerEvent::SwordHit:
                        messageID = Server::Message::PlayerOneDiedFromWeapon;
                        break;
                    case PlayerEvent::TouchedSkelly:
                        messageID = Server::Message::PlayerOneDiedFromSkeleton;
                        break;
                    case PlayerEvent::ExplosionHit:
                        messageID = Server::Message::PlayerOneExploded;
                        break;
                    case PlayerEvent::BeeHit:
                        messageID = Server::Message::PlayerOneDiedFromBees;
                        break;
                    }
                    break;
                case 1:
                    switch (cause)
                    {
                    default: break;
                    case PlayerEvent::Drowned:
                        messageID = Server::Message::PlayerTwoDrowned;
                        break;
                    case PlayerEvent::PistolHit:
                    case PlayerEvent::SwordHit:
                        messageID = Server::Message::PlayerTwoDiedFromWeapon;
                        break;
                    case PlayerEvent::TouchedSkelly:
                        messageID = Server::Message::PlayerTwoDiedFromSkeleton;
                        break;
                    case PlayerEvent::ExplosionHit:
                        messageID = Server::Message::PlayerTwoExploded;
                        break;
                    case PlayerEvent::BeeHit:
                        messageID = Server::Message::PlayerTwoDiedFromBees;
                        break;
                    }
                    break;
                case 2:
                    switch (cause)
                    {
                    default: break;
                    case PlayerEvent::Drowned:
                        messageID = Server::Message::PlayerThreeDrowned;
                        break;
                    case PlayerEvent::PistolHit:
                    case PlayerEvent::SwordHit:
                        messageID = Server::Message::PlayerThreeDiedFromWeapon;
                        break;
                    case PlayerEvent::TouchedSkelly:
                        messageID = Server::Message::PlayerThreeDiedFromSkeleton;
                        break;
                    case PlayerEvent::ExplosionHit:
                        messageID = Server::Message::PlayerThreeExploded;
                        break;
                    case PlayerEvent::BeeHit:
                        messageID = Server::Message::PlayerThreeDiedFromBees;
                        break;
                    }
                    break;
                case 3:
                    switch (cause)
                    {
                    default: break;
                    case PlayerEvent::Drowned:
                        messageID = Server::Message::PlayerFourDrowned;
                        break;
                    case PlayerEvent::PistolHit:
                    case PlayerEvent::SwordHit:
                        messageID = Server::Message::PlayerFourDiedFromWeapon;
                        break;
                    case PlayerEvent::TouchedSkelly:
                        messageID = Server::Message::PlayerFourDiedFromSkeleton;
                        break;
                    case PlayerEvent::ExplosionHit:
                        messageID = Server::Message::PlayerFourExploded;
                        break;
                    case PlayerEvent::BeeHit:
                        messageID = Server::Message::PlayerFourDiedFromBees;
                        break;
                    }
                    break;
                }
                m_sharedData.gameServer->broadcastData(PacketID::ServerMessage, messageID, EP2PSend::k_EP2PSendReliable);

                auto e = data.entity;
                e.getComponent<xy::Transform>().setPosition(e.getComponent<Player>().spawnPosition);
            }

            break;
        case PlayerEvent::Respawned:
            state.action = SceneState::PlayerSpawned;
            m_sharedData.gameServer->broadcastData(PacketID::SceneUpdate, state, EP2PSend::k_EP2PSendReliable);
            break;
        case PlayerEvent::BeeHit:
            state.action = SceneState::Stung;
            m_sharedData.gameServer->broadcastData(PacketID::SceneUpdate, state, EP2PSend::k_EP2PSendReliable);
            break;
        case PlayerEvent::Cursed:
            state.action = data.data == 0 ? SceneState::LostCurse : SceneState::GotCursed;
            m_sharedData.gameServer->broadcastData(PacketID::SceneUpdate, state, EP2PSend::k_EP2PSendReliable);
            break;
        case PlayerEvent::DidAction:
        {
            auto weapon = data.entity.getComponent<Inventory>().weapon;
            if (weapon == Inventory::Pistol)
            {
                for (auto& slot : m_playerSlots)
                {
                    if (slot.gameEntity == data.entity)
                    {
                        slot.stats.shotsFired++;
                        slot.sendStatsUpdate = true;
                        break;
                    }
                }
            }
        }
            break;
        case PlayerEvent::StoleTreasure:
            m_remainingTreasure++;

            //TODO send message packet
            //based on player ID

            //update client scoreboards
            m_sharedData.gameServer->broadcastData(PacketID::TreasureUpdate, std::uint8_t(m_remainingTreasure), EP2PSend::k_EP2PSendReliable);
            LOG(std::to_string(data.entity.getComponent<Player>().playerNumber) + " stole treasure from " + std::to_string(data.data) + "\n", xy::Logger::Type::Info);
            break;
        case PlayerEvent::StashedTreasure:
            m_remainingTreasure--;
            /*if (m_remainingTreasure == 1)
            {
                m_sharedData.gameServer->broadcastData(PacketID::ServerMessage, Server::Message::OneTreasureRemaining, EP2PSend::k_EP2PSendReliable);
            }*/

            auto playerID = data.entity.getComponent<Player>().playerNumber;
            switch (playerID)
            {
            default: break;
            case 0:
                m_sharedData.gameServer->broadcastData(PacketID::ServerMessage, Server::Message::PlayerOneScored, EP2PSend::k_EP2PSendReliable);
                break;
            case 1:
                m_sharedData.gameServer->broadcastData(PacketID::ServerMessage, Server::Message::PlayerTwoScored, EP2PSend::k_EP2PSendReliable);
                break;
            case 2:
                m_sharedData.gameServer->broadcastData(PacketID::ServerMessage, Server::Message::PlayerThreeScored, EP2PSend::k_EP2PSendReliable);
                break;
            case 3:
                m_sharedData.gameServer->broadcastData(PacketID::ServerMessage, Server::Message::PlayerFourScored, EP2PSend::k_EP2PSendReliable);
                break;
            }

            m_playerSlots[playerID].sendStatsUpdate = true;

            //update client scoreboard
            m_sharedData.gameServer->broadcastData(PacketID::TreasureUpdate, std::uint8_t(m_remainingTreasure), EP2PSend::k_EP2PSendReliable);

            //raises an event so client can play a sound
            {
                SceneState state;
                state.action = SceneState::StashedTreasure;
                state.position = data.entity.getComponent<xy::Transform>().getPosition();
                state.serverID = data.entity.getIndex();
                m_sharedData.gameServer->broadcastData(PacketID::SceneUpdate, state, EP2PSend::k_EP2PSendReliable);
            }
            break;
        }
    }
        break;
    case MessageID::ServerMessage:
    {
        const auto& data = msg.getData<ServerEvent>();
        if (data.type == ServerEvent::ClientDisconnected)
        {
            //TODO quit back to idle state if all connections are dropped
            
            CSteamID clientID(data.steamID);
            Actor::ID id = Actor::None;
            
            //remove from active list
            for (auto& slot : m_playerSlots)
            {
                if (slot.clientID == clientID)
                {
                    slot.available = true;
                    slot.clientID = {};
                    slot.gameEntity.getComponent<CSteamID>() = {};
                    slot.gameEntity.getComponent<Inventory>().reset();
                    slot.stats.reset();
                    id = slot.gameEntity.getComponent<Actor>().id;
                    slot.gameEntity.getComponent<Bot>().enabled = true;
                    //slot.gameEntity.getComponent<CollisionComponent>().collidesTerrain = false;
                    break;
                }
            }

            //tell clients which actor just lost its player
            ConnectionState state;
            state.steamID = 0;
            state.actorID = id;
            m_sharedData.gameServer->broadcastData(PacketID::ConnectionUpdate, state, EP2PSend::k_EP2PSendReliable);
        }
    }
        break;
    }

    m_scene.forwardMessage(msg);
}

//private
void RunningState::createScene()
{
    auto& mb = m_sharedData.gameServer->getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::DynamicTreeSystem>(mb);
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<CrabSystem>(mb);
    m_scene.addSystem<CollisionSystem>(mb, true).setTileData(m_mapData.tileData);
    m_scene.addSystem<SkeletonSystem>(mb, m_sharedData, m_pathFinder);
    m_scene.addSystem<CarriableSystem>(mb, m_sharedData);
    m_scene.addSystem<ActorSystem>(mb);
    m_scene.addSystem<PlayerSystem>(mb);
    m_scene.addSystem<BoatSystem>(mb);
    m_scene.addSystem<BeeSystem>(mb);
    m_scene.addSystem<BotSystem>(mb, m_pathFinder, m_sharedData.seedData.hash);
    m_scene.addSystem<CollectibleSystem>(mb, m_sharedData);
    m_scene.addSystem<InventorySystem>(mb, m_sharedData);
    m_scene.addSystem<ParrotLauncherSystem>(mb, m_sharedData);
    m_scene.addSystem<BarrelSystem>(mb, m_sharedData);
    m_scene.addSystem<DecoySystem>(mb);
    m_scene.addSystem<TimedCarriableSystem>(mb);
    m_scene.addSystem<FlareSystem>(mb);
    m_scene.addSystem<SkullShieldSystem>(mb);

    m_scene.addDirector<DigDirector>(m_sharedData);
    m_scene.addDirector<ServerWeaponDirector>(m_sharedData);
    m_scene.addDirector<StormDirector>(m_sharedData);

    m_scene.setSystemActive<BotSystem>(false);

    //set up player slots
    for (auto i = 0u; i < m_playerSlots.size(); ++i)
    {
        createPlayerEntity(i);
    }

    //create crabs n things
    spawnMapActors();
}

void RunningState::spawnPlayer(CSteamID clientID)
{
    //TODO would be nice to let players request a character from lobby menu
    {
        if (clientID.IsValid())
        {
            xy::Entity entity;
            for (auto& slot : m_playerSlots)
            //auto& slot = m_playerSlots[3];
            {
                if (slot.available)
                {
                    slot.available = false;
                    slot.clientID = clientID;
                    slot.stats.reset();
                    entity = slot.gameEntity;
                    entity.getComponent<CSteamID>() = clientID;
                    entity.getComponent<Bot>().enabled = false;
                    //////entity.getComponent<CollisionComponent>().collidesTerrain = true;
                    break;
                }
            }
            entity.getComponent<Inventory>().reset();

            //send player info to client (don't broadcast as this is player specific)
            PlayerInfo info;
            info.actor = entity.getComponent<Actor>();
            info.position = entity.getComponent<xy::Transform>().getPosition();
            m_sharedData.gameServer->sendData(PacketID::PlayerData, info, clientID, EP2PSend::k_EP2PSendReliable);

            //broadcast to all clients that someone has joined
            ConnectionState state;
            state.steamID = clientID.ConvertToUint64();
            state.actorID = entity.getComponent<Actor>().id;
            m_sharedData.gameServer->broadcastData(PacketID::ConnectionUpdate, state, EP2PSend::k_EP2PSendReliable);

            //let everyone know what the current score is
            m_sharedData.gameServer->broadcastData(PacketID::TreasureUpdate, std::uint8_t(m_remainingTreasure), EP2PSend::k_EP2PSendReliable);
        }
        else
        {
            std::cout << "invalid client ID\n";
        }
    }

    //send all the actor data to the client for spawning
    {
        const auto& actors = m_scene.getSystem<ActorSystem>().getActors();
        for (auto actor : actors)
        {
            ActorState state;
            state.position = actor.getComponent<xy::Transform>().getPosition();
            state.actorID = actor.getComponent<Actor>().id;
            state.serverID = actor.getIndex();
            state.serverTime = m_sharedData.gameServer->getServerTime();
            m_sharedData.gameServer->sendData(PacketID::ActorData, state, clientID, EP2PSend::k_EP2PSendReliable);
        }
    }
    m_scene.setSystemActive<BotSystem>(true);
}

void RunningState::createPlayerEntity(std::size_t idx)
{
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Global::BoatPositions[idx] + sf::Vector2f(0.f, Global::PlayerSpawnOffset));
    entity.addComponent<Actor>();
    switch (idx)
    {
    default:
    case 0:
        entity.getComponent<Actor>().id = Actor::PlayerOne;
        break;
    case 1:
        entity.getComponent<Actor>().id = Actor::PlayerTwo;
        break;
    case 2:
        entity.getComponent<Actor>().id = Actor::PlayerThree;
        break;
    case 3:
        entity.getComponent<Actor>().id = Actor::PlayerFour;
        break;
    }
    entity.getComponent<Actor>().entityID = entity.getIndex();
    entity.getComponent<Actor>().serverID = entity.getIndex();
    entity.addComponent<Player>().playerNumber = idx;
    entity.getComponent<Player>().spawnPosition = entity.getComponent<xy::Transform>().getPosition();
    entity.addComponent<InputComponent>();
    entity.addComponent<AnimationModifier>();
    entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
    entity.getComponent<CollisionComponent>().ID = ManifoldID::Player;
    entity.getComponent<CollisionComponent>().collidesBoat = true;
    entity.addComponent<xy::BroadphaseComponent>().setArea(Global::PlayerBounds);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Player | QuadTreeFilter::BotQuery);
    entity.addComponent<Inventory>();
    entity.addComponent<CSteamID>();
    entity.addComponent<Bot>().enabled = true;
    entity.addComponent<Carrier>();

    m_playerSlots[idx].gameEntity = entity;

    //spawn a lantern nearby
    static const std::array<sf::Vector2f, 4u> lampPositions = 
    {
        sf::Vector2f(Global::TileSize / 2.f, Global::TileSize / 2.f),
        sf::Vector2f(-Global::TileSize / 2.f, Global::TileSize / 2.f),
        sf::Vector2f(-Global::TileSize, -Global::TileSize * 1.5f),
        sf::Vector2f(Global::TileSize, -Global::TileSize * 1.5f),
    };

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(SpawnPositions[idx] + lampPositions[idx]);
    entity.addComponent<Actor>().id = Actor::Lantern;
    entity.getComponent<Actor>().entityID = entity.getIndex();
    entity.getComponent<Actor>().serverID = entity.getIndex();
    entity.addComponent<CollisionComponent>().bounds = Global::LanternBounds;
    //entity.getComponent<CollisionComponent>().collidesTerrain = false;
    entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Light | QuadTreeFilter::BotQuery);
    entity.addComponent<AnimationModifier>();
    entity.addComponent<Carriable>().type = Carrier::Flags::Torch;
    entity.getComponent<Carriable>().spawnPosition = entity.getComponent<xy::Transform>().getPosition();
}

void RunningState::spawnMapActors()
{
    const auto& actors = m_islandGenerator.getActorSpawns();
    for (const auto& spawn : actors)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(spawn.position);
        entity.addComponent<Actor>().id = static_cast<Actor::ID>(spawn.id);
        entity.getComponent<Actor>().entityID = entity.getIndex();
        entity.getComponent<Actor>().serverID = entity.getIndex();
        entity.addComponent<AnimationModifier>();

        switch (spawn.id)
        {
        default: break;
        case Actor::Crab:
            entity.addComponent<Crab>().spawnPosition = spawn.position;
            entity.getComponent<Crab>().state = static_cast<Crab::State>(Server::getRandomInt(0, 2));
            entity.getComponent<Crab>().maxTravel += Server::getRandomFloat(-10.f, 10.f);
            entity.getComponent<Crab>().thinkTime += Server::getRandomFloat(-0.5f, 0.5f);
            break;
        case Actor::AmmoSpawn:
        case Actor::TreasureSpawn:
        case Actor::CoinSpawn:
            entity.addComponent<xy::BroadphaseComponent>().setArea({ 0.f, 0.f, Global::TileSize, Global::TileSize });
            entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::WetPatch);
            entity.addComponent<WetPatch>();
            entity.getComponent<Actor>().nonStatic = false;           
            break;
        case Actor::Parrot:
            entity.addComponent<xy::BroadphaseComponent>().setArea({ 0.f, 0.f, Global::TileSize, Global::TileSize });
            entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::ParrotLauncher);
            entity.addComponent<ParrotLauncher>();
            entity.getComponent<Actor>().nonStatic = false;
            //collision is in front so birds appear from behind bushes etc
            entity.addComponent<CollisionComponent>().bounds = { 0.f, Global::TileSize / 2.f, Global::TileSize, Global::TileSize * 4.f };
            entity.getComponent<CollisionComponent>().collidesTerrain = false;
            break;
        case Actor::Bees:
            entity.addComponent<Bee>();
            break;
        }
    }


    for (auto i = 0u; i < Global::BoatPositions.size(); ++i)
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(Global::BoatPositions[i]);
        entity.addComponent<Actor>().id = Actor::ID::Boat;
        entity.getComponent<Actor>().entityID = entity.getIndex();
        entity.getComponent<Actor>().serverID = entity.getIndex();
        entity.getComponent<Actor>().nonStatic = false;
        entity.addComponent<AnimationModifier>();
        entity.addComponent<Boat>().playerNumber = i;
        
        entity.addComponent<CollisionComponent>().bounds = Global::BoatBounds;
        entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Boat;
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::BoatBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Boat | QuadTreeFilter::BotQuery);

        //make boats impassable to path finder
        //TODO make this small enough so that bots can steal treasure
        /*for (auto c : Global::TileExclusionCorners)
        {
            for (auto y = 0; y < Global::TileExclusionX; ++y)
            {
                for (auto x = 0; x < Global::TileExclusionY; ++x)
                {
                    m_pathFinder.addSolidTile({ c.x + x, c.y + y });
                }
            }
        }*/
    }
}

xy::Entity RunningState::spawnActor(sf::Vector2f position, std::int32_t id)
{
    auto entity = m_scene.createEntity();
    entity.addComponent<Actor>().id = static_cast<Actor::ID>(id);
    entity.getComponent<Actor>().entityID = entity.getIndex();
    entity.getComponent<Actor>().serverID = entity.getIndex();
    entity.addComponent<AnimationModifier>();

    switch (id)
    {
    default: break;
    case Actor::ID::Explosion:
    case Actor::ID::Lightning:
        entity.addComponent<CollisionComponent>().bounds = Global::LanternBounds * 2.f;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Explosion;
        entity.addComponent<xy::BroadphaseComponent>().setArea(entity.getComponent<CollisionComponent>().bounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Explosion);
        entity.getComponent<Actor>().nonStatic = false;
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = ExplosionCallback(m_scene);
        break;
    case Actor::ID::DirtSpray:
        entity.getComponent<Actor>().nonStatic = false;
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = ExplosionCallback(m_scene);
        entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(0);
        break;
    case Actor::Barrel:
    {
        auto velocity = position;
        entity.addComponent<Barrel>().velocity = velocity;
        position = {};
        if (velocity.x > 0)
        {
            position.x = -(Global::TileSize * 10.f);
            position.y = Server::getRandomInt(10, Global::TileCountY - 10) * Global::TileSize;
        }
        else if (velocity.x < 0)
        {
            position.x = Global::IslandSize.x + (Global::TileSize * 10.f);
            position.y = Server::getRandomInt(10, Global::TileCountY - 10) * Global::TileSize;
        }
        else if (velocity.y > 0)
        {
            position.y = -(Global::TileSize * 10.f);
            position.x = Server::getRandomInt(10, Global::TileCountX - 10) * Global::TileSize;
        }
        else
        {
            position.y = Global::IslandSize.y + (Global::TileSize * 10.f);
            position.x = Server::getRandomInt(10, Global::TileCountX - 10) * Global::TileSize;
        }

        entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Barrel;
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds * 2.f);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Barrel | QuadTreeFilter::BotQuery);
        entity.addComponent<Inventory>().health = 1;
        //use inventory properties to define what's in the barrel (mostly items)
        if (Server::getRandomInt(0, 4) == 0)
        {
            entity.getComponent<Inventory>().weapon = Barrel::Explosive;
        }
        else
        {
            Server::getRandomInt(0, 6) == 0 ?
                entity.getComponent<Inventory>().weapon = Barrel::Gold :
                entity.getComponent<Inventory>().weapon = Barrel::Item;
        }
    }
        break;
    case Actor::Skeleton:
        entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Skeleton;
        //entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::PlayerBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);
        entity.addComponent<Skeleton>();
        entity.addComponent<Inventory>(); //for heaaalth
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Spawn;
        entity.addComponent<Carrier>();
        break;
    case Actor::Crab:
        entity.addComponent<Crab>().spawnPosition = position;
        entity.getComponent<Crab>().maxTravel += Server::getRandomFloat(-10.f, 10.f);
        break;
    case Actor::Decoy:
        entity.addComponent<Decoy>();
        entity.addComponent<xy::BroadphaseComponent>().setArea({ 0.f, 0.f, Global::TileSize, Global::TileSize });
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Decoy);
        break;
    case Actor::SkullShield:
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::SkullShieldBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);
        entity.addComponent<CollisionComponent>().bounds = Global::SkullShieldBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::SkullShield;
        entity.addComponent<SkullShield>(); //TODO how do we set the owner?
        break;
    case Actor::AmmoSpawn:
    case Actor::TreasureSpawn:
    case Actor::CoinSpawn:
        entity.addComponent<xy::BroadphaseComponent>().setArea({ 0.f, 0.f, Global::TileSize, Global::TileSize });
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::WetPatch);
        entity.addComponent<WetPatch>();
        entity.getComponent<Actor>().nonStatic = false;
        break;
    case Actor::Ammo:
    {
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::CollectibleBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Collectable | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::CollectibleBounds;
        entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<Collectible>().type = Collectible::Type::Ammo;
        entity.getComponent<Collectible>().value = Server::getRandomInt(3, 5);
    }
        break;
    case Actor::Food:
    {
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::CollectibleBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Collectable | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::CollectibleBounds;
        entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<Collectible>().type = Collectible::Type::Food;
        entity.getComponent<Collectible>().value = Server::getRandomInt(12, 22);
    }
    break;
    case Actor::Coin:
    {
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::CollectibleBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Collectable | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::CollectibleBounds;
        entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<Collectible>().type = Collectible::Type::Coin;
        entity.getComponent<Collectible>().value = (Server::getRandomInt(0, 2) == 0) ? 3 : 1;
    }
    break;
    case Actor::Treasure:
        entity.addComponent<CollisionComponent>().bounds = Global::LanternBounds;
        //entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Treasure | QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Flags::Treasure;
        entity.getComponent<Carriable>().spawnPosition = position;
        break;
    case Actor::DecoyItem:
        entity.addComponent<CollisionComponent>().bounds = Global::LanternBounds;
        entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::DecoyItem | QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Flags::Decoy;
        entity.getComponent<Carriable>().spawnPosition = position;
        entity.getComponent<Carriable>().action = [&](xy::Entity e) { spawnActor(e.getComponent<xy::Transform>().getPosition(), Actor::Decoy); };
        entity.addComponent<TimedCarriable>();
        break;
    case Actor::FlareItem:
        entity.addComponent<CollisionComponent>().bounds = Global::LanternBounds;
        entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::FlareItem | QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Flags::Flare;
        entity.getComponent<Carriable>().spawnPosition = position;
        entity.getComponent<Carriable>().offsetMultiplier = Carriable::FlareOffset;
        entity.getComponent<Carriable>().action = [&](xy::Entity e) { spawnActor(e.getComponent<xy::Transform>().getPosition(), Actor::Flare); };
        entity.addComponent<TimedCarriable>();
        break;
    case Actor::SkullItem:
        entity.addComponent<CollisionComponent>().bounds = Global::LanternBounds;
        entity.getComponent<CollisionComponent>().collidesTerrain = false;
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::SkullItem | QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Flags::SpookySkull;
        entity.getComponent<Carriable>().spawnPosition = position;
        //entity.getComponent<Carriable>().offsetMultiplier = Carriable::FlareOffset;
        entity.getComponent<Carriable>().action = [&](xy::Entity e)
        {
            auto shieldEnt = spawnActor(e.getComponent<xy::Transform>().getPosition(), Actor::SkullShield); 
            shieldEnt.getComponent<SkullShield>().owner = e.getComponent<Carriable>().parentEntity.getComponent<Player>().playerNumber;
        };
        entity.addComponent<TimedCarriable>();
        break;
    case Actor::Flare:
        entity.addComponent<Flare>();
        break;
    }

    //adding this last because barrels mutate position
    entity.addComponent<xy::Transform>().setPosition(position);

    ActorState state;
    state.position = entity.getComponent<xy::Transform>().getPosition();
    state.actorID = entity.getComponent<Actor>().id;
    state.serverID = entity.getIndex();
    state.serverTime = m_sharedData.gameServer->getServerTime();
    m_sharedData.gameServer->broadcastData(PacketID::ActorData, state, EP2PSend::k_EP2PSendReliable);

    return entity;
}

void RunningState::endGame()
{
    //tally all stats
    RoundSummary summary;
    for (auto i = 0u; i < m_playerSlots.size(); ++i)
    {
        summary.stats[i].id = m_playerSlots[i].gameEntity.getComponent<Actor>().id;
        summary.stats[i].treasure = m_playerSlots[i].gameEntity.getComponent<Inventory>().treasure;
        summary.stats[i].foesSlain = m_playerSlots[i].stats.foesSlain;
        summary.stats[i].livesLost = m_playerSlots[i].stats.livesLost;
        summary.stats[i].shotsFired = m_playerSlots[i].stats.shotsFired;
        
        m_playerSlots[i].stats.roundXP += summary.stats[i].treasure;
        auto lifeBonus = 5 * XP::LifeBonus;
        for (auto j = 0; j < m_playerSlots[i].stats.livesLost && j < 5; ++j)
        {
            lifeBonus -= XP::LifeBonus;
        }
        m_playerSlots[i].stats.roundXP += lifeBonus;


        summary.stats[i].roundXP = m_playerSlots[i].stats.roundXP;

        m_playerSlots[i].stats.totalXP += summary.stats[i].roundXP;
        m_playerSlots[i].stats.currentLevel = XP::calcLevel(m_playerSlots[i].stats.totalXP);

        //TODO send level and total XP to client
        //might do this per client as no-one strictly needs to know everyone else's xp
    }

    //send stats to clients and notify clients game ended
    m_sharedData.gameServer->broadcastData(PacketID::EndOfRound, summary, EP2PSend::k_EP2PSendReliable);

    //TODO send player stats to steam - but not host if they quit or are a bot

    setNextState(Server::StateID::Active); //set this to active and drop players back into lobby
}

void RunningState::doConCommand(const ConCommand::Data& data)
{
    switch (data.commandID)
    {
    default: break;
    case ConCommand::Weather:
    {
        auto* msg = m_sharedData.gameServer->getMessageBus().post<SceneEvent>(MessageID::SceneMessage);
        msg->type = SceneEvent::WeatherRequested;
        msg->id = data.value.asInt;
    }
        break;
    case ConCommand::Spawn:
        spawnActor(data.position, data.value.asInt);
        break;
    case ConCommand::BotEnable:
        for (auto& slot : m_playerSlots)
        {
            if (slot.available)
            {
                slot.gameEntity.getComponent<Bot>().enabled = static_cast<bool>(data.value.asInt);
            }
        }
        break;
    }
}