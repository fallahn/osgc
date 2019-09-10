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

#include <xyginext/core/Assert.hpp>

#include <vector>
#include <cstring>

namespace PacketID
{
    enum
    {
        None = 0,
        Ping,

        //from server
        LaunchGame, //tell clients the server has launched into game mode
        MapData, //client receive map data
        PlayerData, //contains info about the player for the client
        PlayerUpdate, //update of player data for reconciliation
        ActorData, //generic actor data (remote players, crabs etc)
        ActorUpdate, //update of actors for interpolation
        AnimationUpdate, //server requests new actor animation
        DayNightUpdate, //sets time of day to make client day/night cycles sync
        CarriableUpdate, //something carriable has changed parent
        InventoryUpdate, //player inventory was updated
        SceneUpdate, //usually just that an entity was removed
        ConnectionUpdate, //notifies clients when another client has joined or left the server
        EndOfRound, //game has ended, packet contains summary stats
        ServerMessage, //id of message to display
        RoundTime, //value in seconds of remaining time
        PathData, //list of points of a path used to draw debug output
        PatchSync, //sync data for wet patch
        ParrotLaunch, //server ID of parrot entity to activate
        StatUpdate, //updates the scoreboard
        WeatherUpdate, //uint8 containing current weather mode
        TreasureUpdate, //uint8 number of treasure remaining
        DebugUpdate, //contains state and entity ID
        PhysicsUpdate, //data for client side physics interp
        PhysicsSync, //data to sync physics state on the client

        //from client
        RequestMap, //client is requesting map
        RequestPlayer, //client has loaded map and is requesting a player spawned
        RequestActors, //client requests server send all actors
        ClientInput, //input from client for player controls
        ConCommand, //contains a console command for the server to execute

        //from lobby queries
        RequestSeed, //ask the server for current seed
        SetSeed, //tell the server the current seed
        CurrentSeed, //tell the client the current seed
        StartGame //lobby host wants server to start the game
    };
}