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
#include "ServerGameState.hpp"
#include "ServerLobbyState.hpp"
#include "MessageIDs.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/network/NetData.hpp>

#include <SFML/System/Clock.hpp>

#include <algorithm>
#include <cstring>

using namespace Server;

GameServer::GameServer()
    : m_thread  (&GameServer::run, this),
    m_running   (false),
    m_nextFreeID(0)
{
    m_sharedStateData.gameServer = this;

    const char* defaultSeed = "buns";
    std::strcpy(m_sharedStateData.seedData.str, defaultSeed);

    std::hash<std::string> hash;
    m_sharedStateData.seedData.hash = hash(std::string(defaultSeed));

    m_freeIDs = { 0,1,2,3 };
}

GameServer::~GameServer()
{
    stop();
}

//public
void GameServer::start()
{
    if (!m_host.start("", Global::GamePort, 4, Global::NetworkChannels))
    {
        xy::Logger::log("Failed to start netowrk host :(", xy::Logger::Type::Error);
    }
    else
    {
        xy::Logger::log("Launching Server Instance...", xy::Logger::Type::Info);

        m_currentState = std::make_unique<LobbyState>(m_sharedStateData);
        m_running = true;

        m_thread.launch();
    }
}

void GameServer::stop()
{
    if (m_running)
    {
        m_running = false;
        xy::Logger::log("Stopping Server...", xy::Logger::Type::Info);

        m_thread.wait();

        m_currentState.reset();

        m_freeIDs = { 0,1,2,3, };
        m_nextFreeID = 0;

        xy::Logger::log("Server Stopped.", xy::Logger::Type::Info);
    }
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

void GameServer::clientConnect(const xy::NetEvent& evt)
{
    if (m_currentState->getID() != StateID::Running  //don't join running games
        && m_sharedStateData.connectedClients.size() < m_maxPlayers)
    {
        m_sharedStateData.connectedClients.insert(std::make_pair(evt.peer.getID(), ClientData()));
        m_sharedStateData.connectedClients[evt.peer.getID()].peer = evt.peer;
        m_sharedStateData.connectedClients[evt.peer.getID()].playerID = m_freeIDs[m_nextFreeID++];

        m_host.sendPacket(evt.peer, PacketID::CurrentSeed, m_sharedStateData.seedData, xy::NetFlag::Reliable, Global::ReliableChannel);

        xy::Logger::log("Added user with ID: " + std::to_string(evt.peer.getID()), xy::Logger::Type::Info, xy::Logger::Output::All);

        if (m_sharedStateData.connectedClients.size() == 1)
        {
            //set joiner as host
            m_sharedStateData.hostClient = evt.peer;
        }

        auto* msg = m_messageBus.post<ServerEvent>(MessageID::ServerMessage);
        msg->id = evt.peer.getID();
        msg->type = ServerEvent::ClientConnected;
    }
    else
    {
        //forcefully disconnect the new peer by sending refusal packet
        std::uint8_t reason = (m_sharedStateData.connectedClients.size() == m_maxPlayers) ? 0 : 1;
        m_host.sendPacket(evt.peer, PacketID::RejectClient, reason, xy::NetFlag::Reliable, Global::ReliableChannel);
    }
}

void GameServer::clientDisconnect(const xy::NetEvent& evt)
{
    //getting the ID will return 0 here, so we check the mapping
    std::uint64_t peerID = 0;
    for (const auto& [id, client] : m_sharedStateData.connectedClients)
    {
        if (client.peer == evt.peer)
        {
            peerID = id;
            break;
        }
    }


    auto playerID = m_sharedStateData.connectedClients[peerID].playerID;
    if (m_sharedStateData.connectedClients.size() > 1)
    {
        auto it = std::find(m_freeIDs.begin(), m_freeIDs.end(), playerID);
        *it = m_freeIDs[--m_nextFreeID];
        m_freeIDs[m_nextFreeID] = playerID;
    }
    m_sharedStateData.connectedClients.erase(peerID);

    auto* msg = m_messageBus.post<ServerEvent>(MessageID::ServerMessage);
    msg->id = playerID;
    msg->type = ServerEvent::ClientDisconnected;

    if (m_sharedStateData.connectedClients.empty())
    {
        m_running = false;
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
            case StateID::Lobby:
                m_currentState = std::make_unique<LobbyState>(m_sharedStateData);
                break;
            case StateID::Running:
                m_currentState = std::make_unique<GameState>(m_sharedStateData);
                break;
            }
            m_serverTime.restart();
        }
    }

    m_host.stop();
}