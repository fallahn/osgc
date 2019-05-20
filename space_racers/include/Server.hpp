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

#pragma once

#include "ClientPackets.hpp"

#include <xyginext/core/MessageBus.hpp>
#include <xyginext/network/NetHost.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Thread.hpp>

#include <atomic>
#include <map>
#include <memory>

namespace sv
{
    class State;

    struct SharedData final
    {
        xy::NetHost netHost;
        std::vector<xy::NetPeer> clients;

        //game info set in lobby
        LobbyData lobbyData;
    };

    class Server final
    {
    public:
        Server();
        ~Server();

        Server(const Server&) = delete;
        const Server& operator = (const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator = (Server&&) = delete;

        void run(std::int32_t);
        void quit();

    private:

        std::atomic_bool m_running;

        sf::Time m_netAccumulator;
        sf::Clock m_netClock;

        sf::Time m_updateAccumulator;
        sf::Clock m_updateClock;

        sf::Thread m_thread;
        void threadFunc();

        xy::MessageBus m_messageBus;

        std::unique_ptr<State> m_activeState;
        std::map<std::int32_t, std::function<std::unique_ptr<State>()>> m_stateFactory;
        void registerStates();

        SharedData m_sharedData;
    };
}