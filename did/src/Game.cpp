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

#ifdef STAND_ALONE

#include "Game.hpp"
#include "GameState.hpp"
#include "MenuState.hpp"
#include "ErrorState.hpp"
#include "PauseState.hpp"
#include "Revision.hpp"
#include "Server.hpp"
#include "MessageIDs.hpp"
#include "NetworkClient.hpp"
#include "boxer/boxer.h"
#include "MixerChannels.hpp"

#include "glad/glad.h"

#include <SFML/Window/Event.hpp>

#ifdef DD_DEBUG
extern "C" void __cdecl SteamAPIDebugTextHook(int nSeverity, const char *pchDebugText)
{
    ::OutputDebugString(pchDebugText);

    std::string msg(pchDebugText);
    msg += ", Severity: " + std::to_string(nSeverity);
    xy::Logger::log(msg, xy::Logger::Type::Info/*, xy::Logger::Output::All*/);
}

#include <csignal>
void onAbort(int) 
{
    std::cerr << "Abort signal\n";
}


#endif //DD_DEBUG

Game::Game()
    : xy::App       (sf::ContextSettings(0, 0, 0, 2, 1, 0)),
    m_stateStack    ({ *getRenderWindow(), *this })
{
    if (!gladLoadGL())
    {
        xy::Logger::log("Failed loading opengl functions", xy::Logger::Type::Error);
    }

#ifdef XY_DEBUG
    signal(SIGABRT, onAbort);
#endif
}

Game::~Game()
{

}

//private
void Game::handleEvent(const sf::Event& evt)
{    
#ifdef DD_DEBUG
    if (evt.type == sf::Event::KeyReleased
        && evt.key.code == sf::Keyboard::Escape)
    {
        xy::App::quit();
    }
#endif

    m_stateStack.handleEvent(evt);
}

void Game::handleMessage(const xy::Message& msg)
{    
    /*if (msg.id == MessageID::SystemMessage)
    {
        const auto& data = msg.getData<SystemEvent>();
        if (data.action == SystemEvent::RequestStartServer
            && !m_gameServer.running())
        {
            m_serverThread.launch();

            sf::Clock clock;
            while (!m_gameServer.getSteamID().IsValid()
                && clock.getElapsedTime().asSeconds() < 3.f) {}

            auto newMsg = getMessageBus().post<SystemEvent>(MessageID::SystemMessage);

            if (m_gameServer.getSteamID().IsValid())
            {
                m_sharedData.serverID = m_gameServer.getSteamID();
                xy::Logger::log("Created server instance " + std::to_string(m_sharedData.serverID.ConvertToUint64()), xy::Logger::Type::Info);
                newMsg->action = SystemEvent::ServerStarted;
            }
            else
            {
                m_gameServer.stop();
                xy::Logger::log("Failed creating game server instance", xy::Logger::Type::Error);
                newMsg->action = SystemEvent::ServerStopped;
            }
        }
        else if (data.action == SystemEvent::RequestStopServer)
        {
            m_gameServer.stop();
            m_serverThread.wait();
        }
    }*/

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
    m_sharedData.gameServer = std::make_shared<GameServer>();
    m_sharedData.netClient = std::make_shared<xy::NetClient>();
    m_sharedData.netClient->create(Global::NetworkChannels);

    xy::AudioMixer::setLabel("Sound FX", MixerChannel::FX);
    xy::AudioMixer::setLabel("Music", MixerChannel::Music);

    registerStates();
    m_stateStack.pushState(StateID::Menu);

    getRenderWindow()->setKeyRepeatEnabled(false);

    m_sharedData.gameServer->setMaxPlayers(4);

    xy::App::getActiveInstance()->setWindowTitle("Desert Island Duel");

    return true;
}

void Game::finalise()
{
    if (m_sharedData.netClient && m_sharedData.netClient->connected())
    {
        m_sharedData.netClient->disconnect();
    }

    m_sharedData.gameServer->stop();

    m_stateStack.clearStates();
    m_stateStack.applyPendingChanges();
}

void Game::registerStates()
{
    //TODO intro state

    m_stateStack.registerState<GameState>(StateID::Game, m_sharedData);
    m_stateStack.registerState<MenuState>(StateID::Menu, m_sharedData);
    m_stateStack.registerState<ErrorState>(StateID::Error, m_sharedData);
    m_stateStack.registerState<PauseState>(StateID::Pause, m_sharedData);
}

#endif //STAND_ALONE