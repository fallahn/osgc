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
#include "ResourceIDs.hpp"
#include "MenuState.hpp"
#include "GameState.hpp"
#include "PauseState.hpp"
#include "GameOverState.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>

int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
    *sharedData = std::make_any<SharedData>();

    auto& data = std::any_cast<SharedData&>(*sharedData);
    FontID::handles[FontID::CGA] = data.resources.load<sf::Font>("assets/fonts/IBM_CGA.ttf");
    
#ifndef __linux__
    sf::Image img;
    if (img.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/cursor.png"))
    {
        data.cursor = std::make_unique<sf::Cursor>();
        if (data.cursor->loadFromPixels(img.getPixelsPtr(), img.getSize(), { 0,0 }))
        {
            xy::App::getRenderWindow()->setMouseCursor(*data.cursor);
        }
    }
#endif //cursors are kinda broken on linux

    ss->registerState<MenuState>(StateID::MainMenu, data);
    ss->registerState<GameState>(StateID::Game, data);
    ss->registerState<PauseState>(StateID::Pause, data);
    ss->registerState<GameOverState>(StateID::GameOver, data);

#ifdef XY_DEBUG
    return StateID::Game;
    //return StateID::MainMenu;
#else
    return StateID::MainMenu;
#endif
}

void end(xy::StateStack* ss)
{
    ss->unregisterState(StateID::MainMenu);
    ss->unregisterState(StateID::Game);
    ss->unregisterState(StateID::Pause);
    ss->unregisterState(StateID::GameOver);
}
