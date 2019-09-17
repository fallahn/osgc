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

#ifndef DEMO_INPUT_BINDING_HPP_
#define DEMO_INPUT_BINDING_HPP_

#include <SFML/Window/Keyboard.hpp>

#include <array>

namespace InputFlag
{
    enum
    {
        Up = 0x1,
        Down = 0x2,
        Left = 0x4,
        Right = 0x8,
        CarryDrop = 0x10,
        Action = 0x20,
        WeaponNext = 0x40,
        WeaponPrev = 0x80,
        Strafe = 0x100
    };
}

struct InputBinding final
{
    //buttons come before actions as this indexes into the controller
    //button array as well as the key array
    enum
    {
        ShowMap, ZoomMap, Action, CarryDrop, WeaponNext, WeaponPrev, Strafe, Left, Right, Up, Down, Count
    };
    std::array<sf::Keyboard::Key, Count> keys =
    { 
        sf::Keyboard::M,
        sf::Keyboard::Z,
        sf::Keyboard::Space,
        sf::Keyboard::LControl,
        sf::Keyboard::E,
        sf::Keyboard::Q,
        sf::Keyboard::LShift,
        sf::Keyboard::A,
        sf::Keyboard::D,
        sf::Keyboard::W,
        sf::Keyboard::S        
    };
    std::array<sf::Uint32, 7u> buttons =
    {
        {6, 2, 0, 1, 5, 3, 4} //Back, X, A, B , LB, RB, Y
    };
    sf::Uint32 controllerID = 0;
};

#endif //DEMO_INPUT_BINDING_HPP_
