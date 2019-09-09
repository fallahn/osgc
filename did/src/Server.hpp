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

#include "ServerState.hpp"
#include "ServerSharedStateData.hpp"

#include <xyginext/core/MessageBus.hpp>
#include <xyginext/network/NetHost.hpp>

#include <atomic>
#include <memory>
#include <vector>
#include <cstring>

class GameServer final
{
public:
    GameServer();
    ~GameServer();

    GameServer(const GameServer&) = delete;
    const GameServer& operator = (const GameServer) = delete;
    GameServer(GameServer&&) = delete;
    GameServer& operator = (GameServer&&) = delete;

    void start();
    void stop();
    bool running() const { return m_running; }

    void setMaxPlayers(std::uint8_t maxPlayers) { m_maxPlayers = maxPlayers; }

    inline void sendData(std::uint8_t packetID, void* data, std::size_t size, std::uint64_t destination, xy::NetFlag sendType)
    {
        m_host.sendPacket(m_sharedStateData.connectedClients[destination], packetID, data, size, sendType);
        std::cout << "sending " << size << "bytes\n";
    }

    template <typename T>
    inline void sendData(std::uint8_t packetID, const T& data, std::uint64_t dest, xy::NetFlag sendType = xy::NetFlag::Unreliable);

    template <typename T>
    inline void broadcastData(std::uint8_t packetID, const T& data, xy::NetFlag sendType = xy::NetFlag::Unreliable);

    xy::MessageBus& getMessageBus() { return m_messageBus; }

    std::int32_t getServerTime() const { return m_serverTime.getElapsedTime().asMilliseconds(); }

private:

    std::atomic<bool> m_running;
    std::unique_ptr<Server::State> m_currentState;

    Server::SharedStateData m_sharedStateData;
    xy::MessageBus m_messageBus;

    sf::Clock m_serverTime;
    xy::NetHost m_host;

    std::size_t m_maxPlayers;

    bool pollNetwork(xy::NetEvent&);

    void initServer();
    void closeServer();

    void clientConnect(const xy::NetEvent&);
    void clientDisconnect(const xy::NetEvent&);

    void run();
};

namespace Server
{

}

#include "Server.inl"