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

#ifndef DEMO_PLAYER_INPUT_HPP_
#define DEMO_PLAYER_INPUT_HPP_

#include "InputBinding.hpp"

#include <xyginext/ecs/Entity.hpp>

#include <SFML/Config.hpp>
#include <SFML/System/Clock.hpp>

namespace sf
{
    class Event;
}

namespace xy
{
    class NetClient;
}

struct Input final
{
    std::int32_t timestamp = 0;
    float acceleration = 1.f;
    std::uint16_t mask = 0;
};

struct InputUpdate final //encapsulates packets sent to server
{
    float acceleration = 1.f; //acceleration applied by analogue inputs    
    std::int32_t clientTime = 0; //client timestamp this input was received
    std::uint16_t input = 0; //input mask
    std::uint8_t playerNumber = 0; //player number
};

struct HistoryState final
{
    Input input;
};

using History = std::array<HistoryState, 120u>;

struct InputComponent final
{
    History history;
    std::size_t currentInput = 0; //index into the input array, NOT input flags
    std::size_t lastUpdatedInput = history.size() - 1;
};

/*!
brief Updates the current input mask and applies it to the
current player, along with the client timestamp.
*/
class InputParser final
{
public:
    InputParser(const InputBinding&, xy::NetClient&);

    void handleEvent(const sf::Event&);
    void update(float);

    void setPlayerEntity(xy::Entity, std::uint8_t);
    xy::Entity getPlayerEntity() const;

    void setEnabled(bool);
    bool isEnabled() const { return m_enabled; }

private:

    sf::Uint16 m_currentInput;
    sf::Clock m_clientTimer;

    sf::Uint16 m_prevPad;
    sf::Uint16 m_prevStick;
    float m_analogueAmount;

    std::int32_t m_timeAccumulator;

    bool m_enabled;

    xy::Entity m_playerEntity;
    std::uint8_t m_playerNumber;

    InputBinding m_inputBinding;
    xy::NetClient& m_connection;

    void checkControllerInput();
};

#endif //DEMO_PLAYER_INPUT_HPP_