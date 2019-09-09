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
#include <xyginext/network/NetData.hpp>

#include <SFML/System/Clock.hpp>

#include <algorithm>
#include <cstring>

using namespace Server;

GameServer::GameServer()
    : m_running   (false)
{
    initServer();

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
    closeServer();
}

//public
void GameServer::start()
{
    {
        xy::Logger::log("Launching Server Instance...", xy::Logger::Type::Info);
        m_running = true;

        //don't switch state here - wait until first client has joined and become host
        run();
    }
}

void GameServer::stop()
{
    m_running = false;
    xy::Logger::log("Stopping Server...", xy::Logger::Type::Info);
}

void GameServer::setMaxPlayers(std::uint8_t number)
{
    //TODO can we expose this from enet?
}

//private
bool GameServer::pollNetwork(xy::NetEvent& evt)
{
    std::uint32_t incomingSize = 0;
    if (m_host.pollEvent(evt))
    {
        //handle connect and disconnect
        if (evt.type == xy::NetEvent::ClientConnect)
        {
            clientConnect(evt);
        }
        else if (evt.type == xy::NetEvent::ClientDisconnect)
        {
            clientDisconnect(evt);
        }
        return true;
    }
    return false;
}

void GameServer::initServer()
{
    if (!m_host.start("", Global::GamePort, 4, 10))
    {
        xy::Logger::log("Failed to start Steam Server API", xy::Logger::Type::Error, xy::Logger::Output::All);
        return;
    }

    //small delay to allow time to log on
    sf::Clock clock;
    while(clock.getElapsedTime().asSeconds() < 3.f){}
}

void GameServer::closeServer()
{
    m_host.stop();
}

void GameServer::clientConnect(const xy::NetEvent& evt)
{
    if (m_currentState->getID() != StateID::Running) //don't join running games
    {
        LOG("We're accepting a user connection without validating!! This must be rectified in the future.", xy::Logger::Type::Warning);
        m_sharedStateData.connectedClients.insert(std::make_pair(evt.peer.getID(), evt.peer));

        m_host.sendPacket(evt.peer, PacketID::CurrentSeed, m_sharedStateData.seedData, xy::NetFlag::Reliable);

        xy::Logger::log("Added user with ID: " + std::to_string(evt.peer.getID()), xy::Logger::Type::Info, xy::Logger::Output::All);

        if (m_currentState->getID() == StateID::Idle)
        {
            //set joiner as host
            m_sharedStateData.hostClient = evt.peer;
            m_currentState = std::make_unique<ActiveState>(m_sharedStateData);
        }

        auto* msg = m_messageBus.post<ServerEvent>(MessageID::ServerMessage);
        msg->id = evt.peer.getID();
        msg->type = ServerEvent::ClientConnected;
    }
    else
    {
        std::cout << "Failed to accept client connection, server already in-game\n";
        //TODO forcefully disconnect the new peer by sending refusal packet
    }
}

void GameServer::clientDisconnect(const xy::NetEvent& evt)
{
    /*m_sharedStateData.connectedClients.erase(
        std::remove_if(m_sharedStateData.connectedClients.begin(), m_sharedStateData.connectedClients.end(),
            [&evt](const std::pair<std::uint64_t, xy::NetPeer>& p)
    {
        return evt.peer.getID() == p.first;
    }));*/

    m_sharedStateData.connectedClients.erase(evt.peer.getID());

    auto* msg = m_messageBus.post<ServerEvent>(MessageID::ServerMessage);
    msg->id = evt.peer.getID();
    msg->type = ServerEvent::ClientDisconnected;


    //when game host switches to a different client because
    //the current one left, this is handled by the active state
    if (m_sharedStateData.connectedClients.empty())
    {
        m_sharedStateData.hostClient = {};
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
        xy::NetEvent evt;
        while (pollNetwork(evt))
        {
            if (evt.type == xy::NetEvent::PacketReceived)
            {
                m_currentState->handlePacket(evt);
            }
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