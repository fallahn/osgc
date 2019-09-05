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

#include "Server.hpp"
#include "GlobalConsts.hpp"
#include "Packet.hpp"
#include "ServerRunningState.hpp"
#include "ServerActiveState.hpp"
#include "MessageIDs.hpp"

#include <xyginext/core/Log.hpp>

#include <SFML/System/Clock.hpp>

#include <steam/steam_gameserver.h>

#include <algorithm>
#include <cstring>

using namespace Server;

GameServer::GameServer()
    : m_running   (false)
{
    initSteamServer();

    m_sharedStateData.gameServer = this;
    m_currentState = std::make_unique<IdleState>(m_sharedStateData);

    const char* defaultSeed = "buns";
    std::strcpy(m_sharedStateData.seedData.str, defaultSeed);

    std::hash<std::string> hash;
    m_sharedStateData.seedData.hash = hash(std::string(defaultSeed));
}

GameServer::~GameServer()
{
    stop();
    closeSteamServer();
}

//public
void GameServer::start()
{
    if (SteamGameServer())
    {
        xy::Logger::log("Launching Server Instance...", xy::Logger::Type::Info);
        m_running = true;

        //don't switch state here - wait until first client has joined and become host
        //m_currentState = std::make_unique<Server::ActiveState>(m_sharedStateData);
        run();
    }
    else
    {
        xy::Logger::log("Failed to launch server, Steam services not found", xy::Logger::Type::Error, xy::Logger::Output::All);
    }
}

void GameServer::stop()
{
    m_running = false;
    xy::Logger::log("Stopping Server...", xy::Logger::Type::Info);
}

CSteamID GameServer::getSteamID() const
{
    if (m_running)
    {
        return SteamGameServer()->GetSteamID();
    }
    return {};
}

void GameServer::setMaxPlayers(std::uint8_t number)
{
    SteamGameServer()->SetMaxPlayerCount(number);
}

//private
bool GameServer::pollNetwork(Packet& packet)
{
    std::uint32_t incomingSize = 0;
    if (SteamGameServerNetworking()->IsP2PPacketAvailable(&incomingSize))
    {
        XY_ASSERT(incomingSize > 1, "packet probably too small!");
        if (packet.bytes.size() < incomingSize)
        {
            packet.bytes.resize(incomingSize);
        }

        SteamGameServerNetworking()->ReadP2PPacket(&packet.bytes[0], incomingSize, &incomingSize, &packet.sender);
        packet.size = incomingSize - 1;

        return true;
    }
    return false;
}

void GameServer::initSteamServer()
{
    if (!SteamGameServer_Init(0, Global::AuthPort, Global::GamePort, Global::MasterServerPort, EServerMode::eServerModeAuthenticationAndSecure, "1.0.0.0"))
    {
        xy::Logger::log("Failed to start Steam Server API", xy::Logger::Type::Error, xy::Logger::Output::All);
        return;
    }

    if (SteamGameServer())
    {
        SteamGameServer()->SetModDir("DesertIslandDuel");
        SteamGameServer()->SetProduct("DesertIslandDuel");
        SteamGameServer()->SetGameDescription("Desert Island Duel");
        SteamGameServer()->SetDedicatedServer(true);
        SteamGameServer()->LogOnAnonymous();

        //small delay to allow time to log on
        sf::Clock clock;
        while(clock.getElapsedTime().asSeconds() < 3.f){}
    }
}

void GameServer::closeSteamServer()
{
    if (SteamGameServer())
    {
        SteamGameServer()->LogOff();
        SteamGameServer_Shutdown();
    }
}

void GameServer::p2pSessionRequest(P2PSessionRequest_t* cb)
{
    if (m_currentState->getID() != StateID::Running) //don't join running games
    {
        LOG("We're accepting a user connection without validating!! This must be rectified in the future.", xy::Logger::Type::Warning);
        SteamGameServerNetworking()->AcceptP2PSessionWithUser(cb->m_steamIDRemote);
        m_sharedStateData.connectedClients.emplace_back();
        m_sharedStateData.connectedClients.back().id = cb->m_steamIDRemote;

        m_sharedStateData.gameServer->sendData(PacketID::CurrentSeed, m_sharedStateData.seedData, cb->m_steamIDRemote, EP2PSend::k_EP2PSendReliable);

        xy::Logger::log("Added user with ID: " + std::to_string(cb->m_steamIDRemote.ConvertToUint64()), xy::Logger::Type::Info, xy::Logger::Output::All);

        if (m_currentState->getID() == StateID::Idle)
        {
            //set joiner as host
            m_sharedStateData.gameHost = cb->m_steamIDRemote;
            m_currentState = std::make_unique<ActiveState>(m_sharedStateData);
        }

        auto* msg = m_messageBus.post<ServerEvent>(MessageID::ServerMessage);
        msg->steamID = cb->m_steamIDRemote;
        msg->type = ServerEvent::ClientConnected;
    }
    else
    {
        std::cout << "Failed to accept client connection, server already in-game\n";
    }
}

void GameServer::p2pSessionFail(P2PSessionConnectFail_t* cb)
{
    m_sharedStateData.connectedClients.erase(
        std::remove_if(m_sharedStateData.connectedClients.begin(), m_sharedStateData.connectedClients.end(),
            [cb](const ConnectedClient& cc) 
    {
        return cc.id == cb->m_steamIDRemote;
    }));

    std::string error = "Unknown Reason";
    switch (cb->m_eP2PSessionError)
    {
    default: break;
    case EP2PSessionError::k_EP2PSessionErrorTimeout:
        error = "Client timed out";
        break;
    case EP2PSessionError::k_EP2PSessionErrorMax:
        error = "Firewall error";
        break;
    }
    xy::Logger::log("Dropped client from server, reason: " + error, xy::Logger::Type::Info, xy::Logger::Output::All);

    auto* msg = m_messageBus.post<ServerEvent>(MessageID::ServerMessage);
    msg->steamID = cb->m_steamIDRemote;
    msg->type = ServerEvent::ClientDisconnected;


    //when game host switches to a different client because
    //the current one left, this is handled by the active state
    if (m_sharedStateData.connectedClients.empty())
    {
        m_sharedStateData.gameHost = {};
        m_currentState = std::make_unique<IdleState>(m_sharedStateData);
    }
}

void GameServer::run()
{
    static const float NetworkStep = 1.f / 30.f;
    static const float LogicStep = 1.f / 60.f;

    sf::Clock pingClock;

    sf::Clock networkClock;
    float networkAccumulator = 0.f;

    sf::Clock logicClock;
    float logicAccumulator = 0.f;

    static const std::uint8_t MaxUpdates = 4;
    std::uint8_t getOutClause = MaxUpdates;

    while (m_running)
    {
        if (pingClock.getElapsedTime().asMilliseconds() > 5000)
        {
            //sends a ping to clients
            broadcastData(PacketID::Ping, std::uint8_t(0));
            pingClock.restart();
        }
        
        //do network pump
        Packet packet;
        while (pollNetwork(packet))
        {
            m_currentState->handlePacket(packet);
        }

        //network tick updates (usually broadcasts)
        networkAccumulator += networkClock.restart().asSeconds();
        while (networkAccumulator > NetworkStep
            && getOutClause--)
        {
            networkAccumulator -= NetworkStep;
              
            m_currentState->networkUpdate(NetworkStep);
        }
        getOutClause = MaxUpdates;

        //logic updates
        logicAccumulator += logicClock.restart().asSeconds();
        while (logicAccumulator > LogicStep
            && getOutClause--)
        {
            logicAccumulator -= LogicStep;

            while (!m_messageBus.empty())
            {
                m_currentState->handleMessage(m_messageBus.poll());
            }

            m_currentState->logicUpdate(LogicStep);

            SteamGameServer_RunCallbacks();
        }
        getOutClause = MaxUpdates;

        //check currentState to see if state change was requested
        auto nextState = m_currentState->getNextState();
        if (nextState != m_currentState->getID())
        {
            switch (nextState)
            {
            default: break;
            case StateID::Idle:
                m_currentState = std::make_unique<IdleState>(m_sharedStateData);
                break;
            case StateID::Active:
                m_currentState = std::make_unique<ActiveState>(m_sharedStateData);
                break;
            case StateID::Running:
                m_currentState = std::make_unique<RunningState>(m_sharedStateData);
                break;
            }
            m_serverTime.restart();
        }
    }

    //return to idle when stopped
    m_currentState = std::make_unique<IdleState>(m_sharedStateData);
}