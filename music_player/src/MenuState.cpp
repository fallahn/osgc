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

#include "MainState.hpp"
#include "StateIDs.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

MainState::MainState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State(ss, ctx)
{
    xy::Logger::log("Main state!", xy::Logger::Type::Info);
}

//public
bool MainState::handleEvent(const sf::Event&)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    return true;
}

void MainState::handleMessage(const xy::Message&)
{

}

bool MainState::update(float)
{
    return true;
}

void MainState::draw()
{

}

xy::StateID MainState::stateID() const
{
    return StateID::Main;
}