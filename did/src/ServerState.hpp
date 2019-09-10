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

#include <cstdint>

namespace xy
{
    class Message;
    struct NetEvent;
}

namespace Server
{
    namespace StateID
    {
        enum
        {
            Idle, Lobby, Running
        };
    }
    struct SharedStateData;

    class State
    {
    public:
        virtual ~State() = default;

        virtual std::int32_t getID() const = 0;

        //called at network tick rate (30fps)
        virtual void networkUpdate(float) = 0;

        //called at logic update rate (60fps)
        virtual void logicUpdate(float) = 0;

        //handles recieved packets from network pump
        virtual void handlePacket(const xy::NetEvent&) = 0;

        virtual void handleMessage(const xy::Message&) = 0;

        std::int32_t getNextState() const { return m_nextState; }

    protected:
        void setNextState(std::int32_t nextState) { m_nextState = nextState; }

    private:
        std::int32_t m_nextState = 0;
    };

    class IdleState final : public State
    {
    public:
        explicit IdleState(SharedStateData&);

        std::int32_t getID() const override { return StateID::Idle; }

        void networkUpdate(float) override;

        void logicUpdate(float) override;

        void handlePacket(const xy::NetEvent&) override;

        void handleMessage(const xy::Message&) override;
    };
}