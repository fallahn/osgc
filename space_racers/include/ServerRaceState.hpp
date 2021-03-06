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

#include "ServerStates.hpp"
#include "MapParser.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/network/NetData.hpp>

#include <unordered_map>

struct InputUpdate;


namespace sv
{
    struct ClientConnection final
    {
        xy::NetPeer peer;
        xy::Entity entity;
        bool ready = false; //client has signalled game loaded
        std::uint8_t lapCount = 0;
    };

    class RaceState final : public State
    {
    public:
        RaceState(SharedData&, xy::MessageBus&);
        void handleMessage(const xy::Message&) override;
        void handleNetEvent(const xy::NetEvent&) override;
        void netUpdate(float) override;
        std::int32_t logicUpdate(float) override;
        std::int32_t getID() const override { return StateID::Race; }

    private:
        SharedData& m_sharedData;
        xy::MessageBus& m_messageBus;
        xy::Scene m_scene;
        std::int32_t m_nextState;

        MapParser m_mapParser;

        std::unordered_map<std::uint64_t, ClientConnection> m_players;

        enum GameState
        {
            Prepping, Countdown, Racing, RaceOver, Pause //not paused game, pause between state changes
        }m_state;
        GameState m_unpauseState; //where to next guv?
        sf::Clock m_pauseClock;
        sf::Time m_pauseTime;
        sf::Clock m_countDownTimer;

        sf::Clock m_timeoutClock;
        std::size_t m_finished; //cars that have crossed the line

        std::array<std::uint64_t, 4u> m_racePositions = {}; //peers in the order they crossed the line

        void initScene();
        bool loadMap();
        bool createPlayers();

        void sendPlayerData(const xy::NetPeer&);
        void updatePlayerInput(xy::Entity, const InputUpdate&);
    };
}