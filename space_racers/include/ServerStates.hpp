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

#include <cstdint>

namespace xy
{
	class Message;
}

namespace sv
{
	enum StateID
	{
		Lobby = 1,
		Race = 2
	};

	class State
	{
	public:
		virtual ~State() = default;

		virtual void handleMessage(const xy::Message&) = 0;

		virtual void netUpdate(float) = 0;

		//after each update return a state ID
		//if we return our own then don't switch
		//else return the ID of the state we want
		//to switch the server to
		virtual std::int32_t logicUpdate(float) = 0;

		virtual std::int32_t getID() const = 0;
	};

	class LobbyState final : public State
	{
		void handleMessage(const xy::Message&) override {};
		void netUpdate(float) override {}
		std::int32_t logicUpdate(float) override { return StateID::Lobby; }
		std::int32_t getID() const override { return StateID::Lobby; }
	};

	class RaceState final : public State
	{
		void handleMessage(const xy::Message&) override {};
		void netUpdate(float) override {}
		std::int32_t logicUpdate(float) override { return StateID::Race; }
		std::int32_t getID() const override { return StateID::Race; }
	};
}