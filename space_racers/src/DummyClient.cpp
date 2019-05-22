/*********************************************************************

Copyright 2019 Matt Marchant

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*********************************************************************/

#include "DummyClient.hpp"
#include "MenuState.hpp"
#include "LobbyState.hpp"
#include "RaceState.hpp"
#include "ErrorState.hpp"

DummyClient::DummyClient()
    : m_stateStack(xy::State::Context(m_window, *xy::App::getActiveInstance())),
    m_running(false)
{
    
}

//public
void DummyClient::run()
{
    //create window
    m_window.create({ 800, 600 }, "Window");

    sf::Clock frameClock;
    const sf::Time frameTime = sf::seconds(1.f / 60.f);
    sf::Time accumulator;

    m_running = true;

    initialise();

    while (m_running)
    {
        sf::Event evt;
        while (m_window.pollEvent(evt))
        {
            if (evt.type == sf::Event::Closed)
            {
                m_running = false;
            }

            handleEvent(evt);
        }

        while (!m_messageBus.empty())
        {
            handleMessage(m_messageBus.poll());
        }

        accumulator += frameClock.restart();
        while (accumulator > frameTime)
        {
            accumulator -= frameTime;
            updateApp(frameTime.asSeconds());
        }
        
        m_window.clear();
        draw();
        m_window.display();
    }

    finalise();
}

//private
void DummyClient::handleEvent(const sf::Event& evt)
{
    m_stateStack.handleEvent(evt);
}

void DummyClient::handleMessage(const xy::Message& msg)
{
    m_stateStack.handleMessage(msg);
}

void DummyClient::updateApp(float dt)
{
    m_stateStack.update(dt);
}

void DummyClient::draw()
{
    m_stateStack.draw();
}

bool DummyClient::initialise()
{
    registerStates();
    m_stateStack.pushState(StateID::MainMenu);

    m_window.setView(m_stateStack.updateView());

    return true;
}

void DummyClient::finalise()
{
    m_stateStack.clearStates();
    m_stateStack.applyPendingChanges();
}

void DummyClient::registerStates()
{
    m_stateStack.registerState<MenuState>(StateID::MainMenu, m_sharedData);
    m_stateStack.registerState<LobbyState>(StateID::Lobby, m_sharedData);
    m_stateStack.registerState<RaceState>(StateID::Race, m_sharedData);
    m_stateStack.registerState<ErrorState>(StateID::Error, m_sharedData);
}