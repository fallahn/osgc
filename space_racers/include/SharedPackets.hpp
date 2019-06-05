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

#include <SFML/System/String.hpp>

#include <cstdint>

//this is only used to display other player info
//such as in the lobby or round summary
struct PlayerInfo final
{
    sf::String name = "CPU";
    std::uint32_t vehicle = 0;
    std::uint32_t score = 0; //scores are calculated client side, but based on the race position dictated by server
    std::uint8_t position = 0;
    bool ready = false;
};