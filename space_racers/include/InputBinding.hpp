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

#include <SFML/Window/Keyboard.hpp>

#include <array>

namespace InputFlag
{
    enum
    {
        Left = 0x1,
        Right = 0x2,
        Accelerate = 0x4,
        Brake = 0x8
    };
}

struct InputBinding final
{
    enum
    {
        //accelerate / brake must come first because they also index the joy buttons
        Accelerate, Brake, Left, Right, Count
    };

    std::array<sf::Keyboard::Key, Count> keys = { sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::A, sf::Keyboard::D };
    std::array<sf::Uint32, 2u> buttons = { 0, 1 };
    sf::Uint32 controllerID = 0;
};

