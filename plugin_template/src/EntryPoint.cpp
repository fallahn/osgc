/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#include "PluginExport.hpp"
#include "StateIDs.hpp"
#include "MenuState.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>


int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
    ss->registerState<MenuState>(StateID::MainMenu);
    return StateID::MainMenu;
}

void end(xy::StateStack* ss)
{
    ss->unregisterState(StateID::MainMenu);
}