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

struct LobbyData final
{
    static constexpr std::uint8_t MaxPlayers = 4;    
    std::uint64_t peerIDs[MaxPlayers]; //list of connected peers to map to vehicle type
    std::uint8_t vehicleIDs[MaxPlayers]; //type of vehicle for each player
    std::uint8_t playerCount = 0;
    std::uint8_t mapIndex = 0;
    std::uint8_t lapCount = 0;
    std::uint8_t gameMode = 0;
};

//input taken from the client and sent to the server
//TODO not sure we need this if it's identical to the
//vehicle Input struct?
struct InputUpdate final
{
    float steeringMultiplier = 1.f; //analogue controller multiplier
    float accelerationMultiplier = 1.f;
    std::int32_t timestamp = 0;
    std::uint16_t inputFlags = 0;
};