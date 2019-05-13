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

#include <xyginext/ecs/Scene.hpp>

#include <unordered_map>

namespace xy
{
    struct NetPeer;
}

namespace sv
{
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

        std::unordered_map<std::uint64_t, xy::Entity> m_players;

        void initScene();
        bool loadMap();
        bool createPlayers();

        void sendPlayerData(const xy::NetPeer&);
    };
}