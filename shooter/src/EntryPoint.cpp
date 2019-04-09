/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#include "PluginExport.hpp"
#include "StateIDs.hpp"
#include "ResourceIDs.hpp"
#include "MenuState.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>

#include <SFML/Graphics/Font.hpp>

int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
    *sharedData = std::make_any<SharedData>();

    auto& data = std::any_cast<SharedData&>(*sharedData);
    FontID::handles[FontID::HandDrawn] = data.resources.load<sf::Font>("assets/fonts/HandDrawnShapes.ttf");
    
    ss->registerState<MenuState>(StateID::MainMenu, data);
    return StateID::MainMenu;
}

void end(xy::StateStack* ss)
{
    ss->unregisterState(StateID::MainMenu);
}
