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

#include "Game.hpp"
#include "GameState.hpp"
#include "MenuState.hpp"
#include "ErrorState.hpp"
#include "Revision.hpp"
#include "Server.hpp"
#include "MessageIDs.hpp"
#include "NetworkClient.hpp"
#include "boxer/boxer.h"
#include "MixerChannels.hpp"

//#include "glad/glad.h"

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
    m_serverThread  (&GameServer::start, &m_gameServer),
    m_stateStack    ({ *getRenderWindow(), *this })
{
    /*if (!gladLoadGL())
    {
        xy::Logger::log("Failed loading opengl functions", xy::Logger::Type::Error);
    }*/
    //m_gameServer.setMaxPlayers(4);

#ifdef XY_DEBUG
    signal(SIGABRT, onAbort);
#endif
}

Game::~Game()
{
    //if (m_serverThread)
    {
        m_gameServer.stop();
        m_serverThread.wait();
    }
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
    if (msg.id == MessageID::SystemMessage)
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
                m_sharedData.serverID = {};
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
    }

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
    xy::AudioMixer::setLabel("Sound FX", MixerChannel::FX);
    xy::AudioMixer::setLabel("Music", MixerChannel::Music);
    
    registerStates();
    m_stateStack.pushState(StateID::Menu);

    getRenderWindow()->setKeyRepeatEnabled(false);

    setWindowTitle("Desert Island Duel " + REVISION_STRING);

    if (!SteamAPI_Init())
    {
        boxer::show("Steam not running!\nPlease launch Steam before trying to run the game again.", "Error.");
        return false;
    }

#ifdef DD_DEBUG
    SteamUtils()->SetWarningMessageHook(&SteamAPIDebugTextHook);
#endif //DD_DEBUG
    SteamUtils()->SetOverlayNotificationPosition(ENotificationPosition::k_EPositionTopRight);

    m_gameServer.setMaxPlayers(4);

    return true;
}

void Game::finalise()
{
    m_stateStack.clearStates();
    m_stateStack.applyPendingChanges();

    SteamAPI_Shutdown();
}

void Game::registerStates()
{
    m_stateStack.registerState<GameState>(StateID::Game, m_sharedData);
    m_stateStack.registerState<MenuState>(StateID::Menu, m_sharedData);
    m_stateStack.registerState<ErrorState>(StateID::Error, m_sharedData);
}
