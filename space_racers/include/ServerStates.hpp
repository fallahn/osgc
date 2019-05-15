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

#include <SFML/System/Clock.hpp>

namespace xy
{
    class Message;
    struct NetEvent;
}

namespace sv
{
    struct SharedData;

    enum StateID
    {
        Lobby = 1,
        Race = 2
    };

    class State
    {
    public:
        virtual ~State() = default;

        virtual void handleMessage(const xy::Message&) = 0;

        virtual void handleNetEvent(const xy::NetEvent&) = 0;

        virtual void netUpdate(float) = 0;

        //after each update return a state ID
        //if we return our own then don't switch
        //else return the ID of the state we want
        //to switch the server to
        virtual std::int32_t logicUpdate(float) = 0;

        virtual std::int32_t getID() const = 0;

    protected:
        std::int32_t getServerTime() const { return m_clock.getElapsedTime().asMilliseconds(); }

    private:
        sf::Clock m_clock;
    };
}