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
#ifndef GAME_HPP_
#define GAME_HPP_

#include "Server.hpp"
#include "SharedStateData.hpp"

#include <xyginext/core/App.hpp>

#include <SFML/System/Thread.hpp>

#include <memory>

class Game final : public xy::App
{
public:
    Game();
    ~Game();
    Game(const Game&) = delete;
    Game& operator = (const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator = (Game&&) = delete;

private:

    xy::StateStack m_stateStack;
    SharedData m_sharedData;

    void handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;

    void registerStates() override;
    void updateApp(float dt) override;
    void draw() override;

    bool initialise() override;
    void finalise() override;
};


#endif //GAME_HPP_
#endif //STAND_ALONE