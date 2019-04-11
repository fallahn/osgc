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

#include "Game.hpp"
#include "BrowserState.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/core/FileSystem.hpp>

#include <SFML/Window/Event.hpp>

Game::Game()
    : xy::App   (/*sf::ContextSettings(0, 0, 0, 3, 2, sf::ContextSettings::Core)*/),
    m_stateStack({ *getRenderWindow(), *this })
{

}

Game::~Game()
{
    unloadPlugin();
}

//private
void Game::handleEvent(const sf::Event& evt)
{    
#ifdef XY_DEBUG
    if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == sf::Keyboard::Escape)
        {
            xy::App::quit();
        }
    }
#endif 

    m_stateStack.handleEvent(evt);
}

void Game::handleMessage(const xy::Message& msg)
{    
    m_stateStack.handleMessage(msg);
}

void Game::updateApp(float dt)
{
    m_stateStack.update(dt);
}

void Game::draw()
{
    m_stateStack.draw();
}

bool Game::initialise()
{
    setApplicationName("osgc");
#ifdef XY_DEBUG
    setWindowTitle("OSGC - The Open Source Game Collection (Debug build) - Press F1 for Console");
#else
    setWindowTitle("OSGC - The Open Source Game Collection - Press F1 for Console");
#endif
    
    registerStates();
    m_stateStack.pushState(States::BrowserState);

    getRenderWindow()->setKeyRepeatEnabled(false);
    getRenderWindow()->setView(m_stateStack.updateView());

    return true;
}

void Game::finalise()
{
    m_stateStack.clearStates();
    m_stateStack.applyPendingChanges();
}

void Game::registerStates()
{
    m_stateStack.registerState<BrowserState>(States::BrowserState, *this);
}
