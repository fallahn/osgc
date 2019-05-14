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

#include "LobbyState.hpp"
#include "NetConsts.hpp"
#include "Server.hpp"
#include "ServerStates.hpp"
#include "ClientPackets.hpp"
#include "ServerPackets.hpp"
#include "GameModes.hpp"
#include "VehicleSystem.hpp"

#include <SFML/Window/Event.hpp>

LobbyState::LobbyState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State(ss, ctx),
    m_sharedData(sd)
{
    launchLoadingScreen();

    //launch a local server instance, then connect
    //TODO only do this if we're hosting
    if (!sd.server)
    {
        sd.server = std::make_unique<sv::Server>();
        sd.server->run(sv::StateID::Lobby);
    }

    sf::Clock tempClock;
    while (tempClock.getElapsedTime().asSeconds() < 1.f) {} //give the server time to start

    sd.netClient->connect("127.0.0.1", NetConst::Port);

    quitLoadingScreen();
}

//public
bool LobbyState::handleEvent(const sf::Event& evt)
{
    //temp just to launch a game
    if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == sf::Keyboard::Space)
        {
            LobbyData data;
            data.peerIDs[0] = m_sharedData.netClient->getPeer().getID();
            data.vehicleIDs[0] = Vehicle::Type::Ship;

            m_sharedData.netClient->sendPacket(PacketID::LobbyData, data, xy::NetFlag::Reliable);

            //TODO we need to ignore further input from other clients to the lobby
            //perhaps relay this from the server when it rx lobby data
        }
    }

    return true;
}

void LobbyState::handleMessage(const xy::Message&)
{

}

bool LobbyState::update(float)
{
    xy::NetEvent evt;
    while (m_sharedData.netClient->pollEvent(evt))
    {
        if (evt.type == xy::NetEvent::PacketReceived)
        {
            const auto& packet = evt.packet;
            switch (packet.getID())
            {
            default: break;
            case PacketID::GameStarted:
                //set shared data with game info
                //and launch game state
            {
                auto data = packet.as<GameStart>();
                m_sharedData.gameData.playerCount = data.playerCount;
                m_sharedData.gameData.mapIndex = data.mapIndex;

                switch (data.gameMode)
                {
                default: break; //only network races implemented...
                case GameMode:: Race:
                    requestStackClear();
                    requestStackPush(StateID::Race);
                    break;
                }
            }
                break;
            case PacketID::ErrorServerMap:
                LOG("Server failed to laod map - TODO report map index which failed", xy::Logger::Type::Error);
                //TODO push an error state here with a meaningful message
                requestStackClear();
                requestStackPush(StateID::MainMenu);
                break;
            }
        }
    }

    return true;
}

void LobbyState::draw()
{

}