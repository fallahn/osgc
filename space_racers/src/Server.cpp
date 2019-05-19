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
#include "ServerStates.hpp"
#include "ServerLobbyState.hpp"
#include "ServerRaceState.hpp"
#include "NetConsts.hpp"

#include <xyginext/network/NetData.hpp>

#include <SFML/System/Sleep.hpp>

#include <iostream>

using namespace sv;

namespace
{
    const sf::Time NetTime = sf::seconds(1.f / 20.f);
    const sf::Time UpdateTime = sf::seconds(1.f / 60.f);
}

Server::Server()
    : m_running    (false),
    m_thread    (&Server::threadFunc, this)
{
    registerStates();
}

Server::~Server()
{
    quit();
}

//public
void Server::run(std::int32_t firstState)
{
    if (!m_running)
    {
        m_activeState = m_stateFactory[firstState]();
        m_thread.launch();
    }
}

void Server::quit()
{
    if (m_running)
    {
        LOG("Quitting server...", xy::Logger::Type::Info);
        m_running = false;
        m_thread.wait();
    }
}

//private
void Server::threadFunc()
{
    LOG("Server launched!", xy::Logger::Type::Info);

    m_running = m_sharedData.netHost.start("", NetConst::Port, 4, 2);

    m_netClock.restart();
    m_netAccumulator = sf::Time::Zero;

    m_updateClock.restart();
    m_updateAccumulator = sf::Time::Zero;

    //use this to save a bit of processing on the server thread
    //when no updates have been performed
    std::int32_t updateCount = 0;

    while (m_running)
    {
        xy::NetEvent evt;
        while (m_sharedData.netHost.pollEvent(evt))
        {
            if (evt.type == xy::NetEvent::ClientConnect)
            {
                //TODO only allow new connections when in a lobby
                LOG("Only allow new connections from lobby!", xy::Logger::Type::Warning);
                m_sharedData.clients.push_back(evt.peer);
            }
            else if (evt.type == xy::NetEvent::ClientDisconnect)
            {
                m_sharedData.clients.erase(std::remove_if(
                    m_sharedData.clients.begin(),
                    m_sharedData.clients.end(),
                    [&](const xy::NetPeer peer)
                    {
                        return evt.peer == peer;
                    }),
                    m_sharedData.clients.end());
            }

            m_activeState->handleNetEvent(evt);
        }

        m_netAccumulator += m_netClock.restart();
        while (m_netAccumulator > NetTime)
        {
            //do net update
            m_activeState->netUpdate(NetTime.asSeconds());
            m_netAccumulator -= NetTime;

            updateCount++;
        }

        std::int32_t stateResult = m_activeState->getID();
        m_updateAccumulator += m_updateClock.restart();
        while (m_updateAccumulator > UpdateTime)
        {
            while (!m_messageBus.empty())
            {
                m_activeState->handleMessage(m_messageBus.poll());
            }

            //do logic update
            stateResult = m_activeState->logicUpdate(UpdateTime.asSeconds());
            m_updateAccumulator -= UpdateTime;

            updateCount++;
        }

        if (stateResult != m_activeState->getID())
        {
            m_activeState = m_stateFactory[stateResult]();
            updateCount++;
        }

        //sleep the thread to save some CPU
        if (updateCount == 0)
        {
            sf::sleep((UpdateTime - m_updateAccumulator) / 2.f);
        }
        updateCount = 0;
    }

    //tidy up
    m_sharedData.netHost.stop();
    m_sharedData.clients.clear();

    LOG("Server quit!", xy::Logger::Type::Info);
}

void Server::registerStates()
{
    m_stateFactory[sv::StateID::Lobby] =
        [&]()->std::unique_ptr<State>
    {
        return std::make_unique<LobbyState>(m_sharedData, m_messageBus);
    };

    m_stateFactory[sv::StateID::Race] =
        [&]()->std::unique_ptr<State>
    {
        return std::make_unique<RaceState>(m_sharedData, m_messageBus);
    };
}