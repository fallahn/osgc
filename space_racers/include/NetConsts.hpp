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
#include <cstddef>

#include <SFML/System/Clock.hpp>

namespace NetConst
{
    static const std::uint16_t Port = 4330;
    static const std::size_t MaxNameSize = 20 * sizeof(sf::Uint32); //bytes
    static const sf::Time PingTimeout = sf::seconds(12.f);
}

namespace PacketID
{
    enum
    {
        LobbyData, //contains settings about the game from the lobby
        LeftLobby, //peer ID of player who left the lobby
        ReadyStateToggled, //0 if not ready, 1 if ready (uint8)
        LapCountChanged, //decrease if 0, else increase (uint8)
        MapChanged, //decrease index if 0, else increase (uint8)
        VehicleChanged, //client has changed vehicle
        RequestPlayerName, //lobby wants the player's name
        DeliverPlayerData, //server is sending other player data to clients
        DeliverPlayerName, //server is sending name of particular player
        DeliverMapName, //lobby is sending map name
        NameString, //packet is utf32 coded name string from client
        LaunchGame, //lobby host has indicated they wish to start
        GameStarted, //sends a GameStarted packet
        ClientMapLoaded, //clients has loaded map, requesting vehicles
        VehicleData, //packet contains data for a single vehicle
        ActorData, //packet contains actor data to spawn on client
        ClientReady, //client has loaded all actors and is ready to start
        CountdownStarted, //start count-in
        RaceStarted, //countdown has completed
        RaceTimerStarted, //first player has crossed the line
        RaceFinished, //all players crossed the line or were eliminated
        ClientLeftRace, //server ent ID of leaving client for tidy up
        ClientPing, //stop the client timeing out (also updates lobby browser)

        VehicleExploded, //server id of exploded vehicle
        VehicleFell, //server id of falling vehicle
        VehicleSpawned, //server id of respawning vehicle
        LapLine, //vehicle with this server ID crossed the ling (uint32)

        ErrorServerMap, //server failed to load map
        ErrorServerGeneric, //generic server error
        ErrorServerFull, //lobby is full or already in game
        ErrorServerDisconnect, //server was closed, clients should disconnect

        ClientInput, //packet contains controller input from client (InputUpdate)
        ClientUpdate, //update for the client to reconcile (from server)
        ActorUpdate, //actor update for client side interpolation
        VehicleActorUpdate, //vehicle update for client side interpolation

        DebugPosition, //packet contains position info for debug
    };
}