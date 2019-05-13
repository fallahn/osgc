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

#include "ServerRaceState.hpp"

using namespace sv;

RaceState::RaceState(SharedData& sd)
    : m_shareData(sd)
{

}

//public
void RaceState::handleMessage(const xy::Message&)
{

}

void RaceState::handleNetEvent(const xy::NetEvent&)
{

}

void RaceState::netUpdate(float)
{

}

std::int32_t RaceState::logicUpdate(float)
{

    return StateID::Race;
}

