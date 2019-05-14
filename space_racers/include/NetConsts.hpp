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

namespace NetConst
{
    const std::uint16_t Port = 4330;
}

namespace PacketID
{
    enum
    {
        LobbyData, //contains settings about the game from the lobby owner
        GameStarted, //sends a GameStarted packet
        ClientMapLoaded, //clients has loaded map, requesting vehicles
        VehicleData, //packet contains data for a single vehicle
        ActorData, //packet contains actor data to spawn on client
        ClientVehiclesLoaded, //client has loaded all vehicles and is ready to start
        RaceStarted, //countdown has triggered


        ErrorServerMap, //server failed to load map
        ErrorServerGeneric, //generic server error

        ClientInput, //packet contains controller input from client (InputUpdate)
        ClientUpdate, //update for the client to reconcile (from server)

        DebugPosition, //packet contains position info for debug
    };
}