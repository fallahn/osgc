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

#pragma once

//create a dummy client to connect to local host for network testing

#include "StateIDs.hpp"

#include <xyginext/core/App.hpp>

#include <SFML/Graphics/RenderWindow.hpp>

class DummyClient final// : public xy::App
{
public:
    DummyClient();

    void run();
    void quit() { m_running = false; }

private:
    SharedData m_sharedData;

    sf::RenderWindow m_window;
    xy::StateStack m_stateStack;
    xy::MessageBus m_messageBus;

    bool m_running;

    void handleEvent(const sf::Event&);// override;
    void handleMessage(const xy::Message&);// override;

    void registerStates();// override;
    void updateApp(float dt);// override;
    void draw();// override;

    bool initialise();// override;
    void finalise();// override;

};

#include <SFML/System/Thread.hpp>

class ClientLauncher final
{
public:
    ClientLauncher();

    void launch();
    void quit();

private:
    sf::Thread m_thread;
    DummyClient m_client;

    void threadFunc();
};