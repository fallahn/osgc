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
#include "MenuState.hpp"
#include "RaceState.hpp"
#include "DebugState.hpp"
#include "LobbyState.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>


int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
    *sharedData = std::make_any<SharedData>();

    auto& data = std::any_cast<SharedData&>(*sharedData);

    //TODO we probably don't want to create a client until
    //the user chooses 'netplay' from the menu
    data.netClient = std::make_unique<xy::NetClient>();
    if (!data.netClient->create(2))
    {
        xy::Logger::log("Creating net client failed...", xy::Logger::Type::Error);
    }

    ss->registerState<MenuState>(StateID::MainMenu, data);
    ss->registerState<RaceState>(StateID::Race, data);
    ss->registerState<DebugState>(StateID::Debug, data);
    ss->registerState<LobbyState>(StateID::Lobby, data);

#ifdef XY_DEBUG
    return StateID::Debug;
    //return StateID::Lobby;
    //return StateID::MainMenu;
#else
    //return StateID::MainMenu;
    return StateID::Lobby;
#endif
}

void end(xy::StateStack* ss)
{
    ss->unregisterState(StateID::MainMenu);
    ss->unregisterState(StateID::Race);
    ss->unregisterState(StateID::Debug);
    ss->unregisterState(StateID::Lobby);
}