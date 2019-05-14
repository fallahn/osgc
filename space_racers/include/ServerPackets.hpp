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

struct GameStart final
{
    std::uint8_t playerCount = 1; //so the client knows it has received all player info
    std::uint8_t mapIndex = 0;
    std::uint8_t gameMode = 0;
};

struct VehicleData final
{
    std::uint64_t peerID = 0;
    std::uint32_t serverEntityID = 0; //for clients side tracking when receiving updates
    float x = 0.f; float y = 0.f; //initial position
    std::uint8_t vehicleType = 0;
};

//server state sent for reconciliation
struct ClientUpdate final
{
    float x = 0.f;
    float y = 0.f;
    float rotation = 0.f;
    float velX = 0.f;
    float velY = 0.f;
    float velRot = 0.f;
    std::int32_t clientTimestamp = 0;
};