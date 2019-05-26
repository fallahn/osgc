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

//player data shared by lobby
//names are sent separately
struct PlayerData final
{
    std::uint64_t peerID = 0;
    bool ready = false;
    std::uint8_t vehicle = 0;

};

struct PlayerIdent final
{
    std::uint64_t peerID = 0;
    std::uint32_t serverID = 0;
};

struct GameStart final
{
    std::uint8_t actorCount = 1; //so the client knows it has received all actor info
    std::uint8_t mapIndex = 0;
    std::uint8_t gameMode = 0;
};

struct VehicleData final
{
    float x = 0.f; float y = 0.f; //initial position
    std::int32_t serverID = 0;
    std::uint8_t vehicleType = 0;
    std::uint8_t colourID = 0; //index into colour array
};

//tells the client to spawn an actor
struct ActorData final
{
    float x = 0.f;
    float y = 0.f;
    float rotation = 0.f;
    float scale = 1.f;
    std::int32_t timestamp = 0;
    std::int16_t serverID = 0;
    std::int16_t actorID = 0;
    std::uint8_t colourID = 0;
};

struct ActorUpdate final
{
    float x = 0.f;
    float y = 0.f;
    float velX = 0.f;
    float velY = 0.f;
    float rotation = 0.f;
    std::int32_t timestamp = 0;
    std::uint16_t serverID = 0;
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
    std::uint16_t collisionFlags = 0;
    std::uint16_t stateFlags = 1;
};