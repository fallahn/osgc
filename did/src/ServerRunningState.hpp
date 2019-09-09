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
#include "IslandGenerator.hpp"
#include "PlayerStats.hpp"
#include "PathFinder.hpp"
#include "ServerRoundTimer.hpp"

#include <xyginext/ecs/Scene.hpp>

namespace Server
{
    namespace ConCommand
    {
        struct Data;
    }

    class RunningState final : public State
    {
    public:
        explicit RunningState(SharedStateData&);

        std::int32_t getID() const override { return StateID::Running; }

        void networkUpdate(float) override;

        void logicUpdate(float) override;

        void handlePacket(const xy::NetEvent&) override;

        void handleMessage(const xy::Message&) override;

    private:
        SharedStateData& m_sharedData;
        IslandGenerator m_islandGenerator;
        MapData m_mapData;

        xy::Scene m_scene;

        sf::Clock m_dayNightClock;
        float m_dayNightUpdateTime;

        struct PlayerSlot final
        {
            xy::Entity gameEntity;
            std::uint64_t clientID;
            bool available = true;
            PlayerStats stats;
            bool sendStatsUpdate = true;
        };
        std::array<PlayerSlot, 4u> m_playerSlots;
        std::size_t m_remainingTreasure;
        RoundTimer m_roundTimer;

        PathFinder m_pathFinder;

        void createScene();
        void spawnPlayer(std::uint64_t);
        void createPlayerEntity(std::size_t);
        void spawnMapActors();
        xy::Entity spawnActor(sf::Vector2f, std::int32_t); //also broadcasts to clients
        void endGame();

        void doConCommand(const Server::ConCommand::Data&);
    };
}
