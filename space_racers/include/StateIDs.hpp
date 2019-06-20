/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#pragma once


#include "InputBinding.hpp"
#include "Server.hpp"
#include "LoadingScreen.hpp"

#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/network/NetClient.hpp>

#include <memory>
#include <string>

namespace StateID
{
    enum
    {
        MainMenu,
        Lobby,
        Race,
        Debug,
        Error,
        Pause,
        Summary,
        LocalRace,
        LocalElimination,
        TimeTrial,
        TimeTrialSummary
    };
}

struct GameData final
{
    std::uint8_t actorCount = 1;
    std::uint8_t lapCount = 3;
};

struct LocalPlayer final
{
    std::uint32_t vehicle = 0;
    InputBinding inputBinding;
    std::uint8_t lapCount = 0;
    std::uint8_t position = 0;
    std::uint8_t points = 0;
    bool cpu = true;
};

struct SharedData final
{
    LoadingScreen loadingScreen;

#ifdef LO_SPEC
    bool useBloom = false;
#else
    bool useBloom = true; //for disabling expensive gfx processing
#endif

    xy::ResourceHandler resources;
    std::size_t fontID = 0;

    //used in all game modes
    std::string errorMessage;
    GameData gameData;

    std::string mapName = "AceOfSpace.tmx";
    std::array<std::uint64_t, 4u> racePositions = {};

    //local game data
    std::array<LocalPlayer, 4u> localPlayers;
    
    //time trial data
    std::vector<float> lapTimes;
    float trackRecord = 0.f;

    //netgame data
    bool hosting = true;
    sf::String ip = "127.0.0.1";
    sf::String name = "Player";
    //used in lobby
    std::map<std::uint64_t, PlayerInfo> playerInfo;
    
    //hack to allow non-copyable member in std::any
    //please don't pass copies of this around...
    std::shared_ptr<xy::NetClient> netClient;
    //only used if we're hosting, else nullptr
    std::shared_ptr<sv::Server> server;
};