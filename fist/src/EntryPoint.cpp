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
#include "GameState.hpp"
#include "ErrorState.hpp"
#include "ModelState.hpp"
#include "SharedStateData.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>

#include <SFML/OpenGL.hpp>

int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
    *sharedData = std::make_any<SharedData>();
    auto& data = std::any_cast<SharedData&>(*sharedData);

    ss->registerState<MenuState>(StateID::MainMenu);
    ss->registerState<GameState>(StateID::Game, data);
    ss->registerState<ErrorState>(StateID::Error);
    ss->registerState<ModelState>(StateID::Model, data);

    xy::App::getActiveInstance()->setWindowTitle("DoodleDude 2 - Bob's Big Adventure");

    //return StateID::Game;
    return StateID::Model;
}

void end(xy::StateStack* ss)
{
    ss->unregisterState(StateID::Model);
    ss->unregisterState(StateID::Error);
    ss->unregisterState(StateID::Game);
    ss->unregisterState(StateID::MainMenu);
}