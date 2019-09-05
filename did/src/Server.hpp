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
#include <steam/steam_gameserver.h>

#include <atomic>
#include <memory>
#include <vector>
#include <cstring>

struct Packet;
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

    CSteamID getSteamID() const;
    void setMaxPlayers(std::uint8_t);

    template <typename T>
    void sendData(std::uint8_t packetID, const T& data, CSteamID dest, EP2PSend sendType = k_EP2PSendUnreliable);

    template <typename T>
    void broadcastData(std::uint8_t packetID, const T& data, EP2PSend sendType = k_EP2PSendUnreliable);

    xy::MessageBus& getMessageBus() { return m_messageBus; }

    std::int32_t getServerTime() const { return m_serverTime.getElapsedTime().asMilliseconds(); }

private:

    std::atomic<bool> m_running;
    std::unique_ptr<Server::State> m_currentState;

    Server::SharedStateData m_sharedStateData;
    xy::MessageBus m_messageBus;

    sf::Clock m_serverTime;

    bool pollNetwork(Packet&);

    void initSteamServer();
    void closeSteamServer();

    STEAM_GAMESERVER_CALLBACK(GameServer, p2pSessionRequest, P2PSessionRequest_t);
    STEAM_GAMESERVER_CALLBACK(GameServer, p2pSessionFail, P2PSessionConnectFail_t);

    void run();
};

namespace Server
{

}

#include "Server.inl"