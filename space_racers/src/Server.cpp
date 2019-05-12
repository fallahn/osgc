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

#include "Server.hpp"
#include "ServerStates.hpp"

#include <iostream>

using namespace sv;

namespace
{
	const sf::Time NetTime = sf::seconds(1.f / 10.f);
	const sf::Time UpdateTime = sf::seconds(1.f / 60.f);
}

Server::Server()
	: m_running	(false),
	m_thread	(&Server::threadFunc, this)
{
	registerStates();
}

Server::~Server()
{
	quit();
}

//public
void Server::run(std::int32_t firstState)
{
	if (!m_running)
	{
		m_activeState = m_stateFactory[firstState]();

		m_running = true;
		m_thread.launch();
	}
}

void Server::quit()
{
	if (m_running)
	{
		std::cout << "Quitting server...\n";
		m_running = false;
		m_thread.wait();
	}
}

//private
void Server::threadFunc()
{
	std::cout << "Server launched!\n";

	m_netClock.restart();
	m_netAccumulator = sf::Time::Zero;

	m_updateClock.restart();
	m_updateAccumulator = sf::Time::Zero;

	while (m_running)
	{
		//TODO poll incoming packets and pass to active state

		while (!m_messageBus.empty())
		{
			m_activeState->handleMessage(m_messageBus.poll());
		}

		m_netAccumulator += m_netClock.restart();
		while (m_netAccumulator > NetTime)
		{
			//do net update
			m_activeState->netUpdate(NetTime.asSeconds());
			m_netAccumulator -= NetTime;
		}

		std::int32_t stateResult = 0;
		m_updateAccumulator += m_updateClock.restart();
		while (m_updateAccumulator > UpdateTime)
		{
			//do logic update
			stateResult = m_activeState->logicUpdate(UpdateTime.asSeconds());
			m_updateAccumulator -= UpdateTime;
		}

		if (stateResult != m_activeState->getID())
		{
			m_activeState = m_stateFactory[stateResult]();
		}

	}

	//TODO tidy up

	std::cout << "Server quit!\n";
}

void Server::registerStates()
{
	m_stateFactory[sv::StateID::Lobby] =
		[]()->std::unique_ptr<State>
	{
		return std::make_unique<LobbyState>();
	};

	m_stateFactory[sv::StateID::Race] =
		[]()->std::unique_ptr<State>
	{
		return std::make_unique<RaceState>();
	};
}