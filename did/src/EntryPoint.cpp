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
#include "PauseState.hpp"

#include "glad/glad.h"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>

#ifdef DD_DEBUG
#include <csignal>
void onAbort(int)
{
    std::cerr << "Abort signal\n";
}


#endif //DD_DEBUG

namespace
{
    std::shared_ptr<GameServer> gameServer;
    std::shared_ptr<xy::NetClient> netClient;
}

int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
#ifdef XY_DEBUG
    signal(SIGABRT, onAbort);
#endif
    
    if (!gladLoadGL())
    {
        xy::Logger::log("Failed to load OpenGL");
        return StateID::ParentState;
    }

    *sharedData = std::make_any<SharedData>();
    auto& sd = std::any_cast<SharedData&>(*sharedData);

    sd.gameServer = std::make_shared<GameServer>();
    sd.netClient = std::make_shared<xy::NetClient>();
    sd.netClient->create(Global::NetworkChannels);

    gameServer = sd.gameServer;
    netClient = sd.netClient;

    xy::AudioMixer::setLabel("Sound FX", MixerChannel::FX);
    xy::AudioMixer::setLabel("Music", MixerChannel::Music);

    ss->registerState<MenuState>(StateID::Menu, sd);
    ss->registerState<ErrorState>(StateID::Error, sd);
    ss->registerState<GameState>(StateID::Game, sd);
    ss->registerState<PauseState>(StateID::Pause, sd);

    gameServer->setMaxPlayers(4);

    return StateID::Menu;
}

void end(xy::StateStack* ss)
{
    netClient->disconnect();

    gameServer->stop();

    gameServer.reset();
    netClient.reset();

    ss->unregisterState(StateID::Menu);
    ss->unregisterState(StateID::Error);
    ss->unregisterState(StateID::Game);
    ss->unregisterState(StateID::Pause);

#ifdef XY_DEBUG
    signal(SIGABRT, SIG_DFL);
#endif
}