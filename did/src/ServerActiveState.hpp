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

#include <xyginext/ecs/Scene.hpp>

class PhysicsSystem;
namespace Server
{
    class ActiveState final : public State
    {
    public:
        explicit ActiveState(SharedStateData&);
        ~ActiveState();

        std::int32_t getID() const override { return StateID::Active; }

        void networkUpdate(float) override;

        void logicUpdate(float) override;

        void handlePacket(const Packet&) override;

        void handleMessage(const xy::Message&) override;

    private:
        SharedStateData& m_sharedData;
    };
}