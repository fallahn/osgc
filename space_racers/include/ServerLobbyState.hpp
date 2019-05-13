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

namespace sv
{
    class LobbyState final : public State
    {
    public:
        explicit LobbyState(SharedData&);
        void handleMessage(const xy::Message&) override;
        void handleNetEvent(const xy::NetEvent&) override;
        void netUpdate(float) override;
        std::int32_t logicUpdate(float) override;
        std::int32_t getID() const override { return StateID::Lobby; }

    private:
        SharedData& m_sharedData;
    };
}
