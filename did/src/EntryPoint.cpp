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

#include "PluginExport.hpp"
#include "StateIDs.hpp"
#include "GameState.hpp"
#include "MenuState.hpp"
#include "ErrorState.hpp"
#include "Server.hpp"
#include "boxer/boxer.h"
#include "MixerChannels.hpp"
#include "SharedStateData.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>

#ifdef DD_DEBUG
extern "C" void __cdecl SteamAPIDebugTextHook(int nSeverity, const char* pchDebugText)
{
    ::OutputDebugString(pchDebugText);

    std::string msg(pchDebugText);
    msg += ", Severity: " + std::to_string(nSeverity);
    xy::Logger::log(msg, xy::Logger::Type::Info/*, xy::Logger::Output::All*/);
}

#include <csignal>
void onAbort(int)
{
    std::cerr << "Abort signal\n";
}


#endif //DD_DEBUG

namespace
{
    std::shared_ptr<GameServer> gameServer;
    std::shared_ptr<sf::Thread> serverThread;
}

int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
#ifdef XY_DEBUG
    signal(SIGABRT, onAbort);
#endif
    
    *sharedData = std::make_any<SharedData>();
    auto& sd = std::any_cast<SharedData&>(*sharedData);

    sd.gameServer = std::make_shared<GameServer>();
    sd.serverThread = std::make_shared<sf::Thread>(&GameServer::start, sd.gameServer.get());

    gameServer = sd.gameServer;
    serverThread = sd.serverThread;

    xy::AudioMixer::setLabel("Sound FX", MixerChannel::FX);
    xy::AudioMixer::setLabel("Music", MixerChannel::Music);

    ss->registerState<MenuState>(StateID::Menu, sd);
    ss->registerState<ErrorState>(StateID::Error, sd);
    ss->registerState<GameState>(StateID::Game, sd);

    if (!SteamAPI_Init())
    {
        boxer::show("Steam not running!\nPlease launch Steam before trying to run the game again.", "Error.");
        return StateID::ParentState;
    }

    gameServer->setMaxPlayers(4);

#ifdef DD_DEBUG
    SteamUtils()->SetWarningMessageHook(&SteamAPIDebugTextHook);
#endif //DD_DEBUG
    SteamUtils()->SetOverlayNotificationPosition(ENotificationPosition::k_EPositionTopRight);

    return StateID::Menu;
}

void end(xy::StateStack* ss)
{
    gameServer->stop();
    serverThread->wait();

    gameServer.reset();
    serverThread.reset();

    SteamAPI_Shutdown();

    ss->unregisterState(StateID::Menu);
    ss->unregisterState(StateID::Error);
    ss->unregisterState(StateID::Game);

#ifdef XY_DEBUG
    signal(SIGABRT, SIG_DFL);
#endif
}