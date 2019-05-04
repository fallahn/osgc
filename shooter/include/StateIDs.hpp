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

#include "LoadingScreen.hpp"
#include <xyginext/resources/ResourceHandler.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace StateID
{
    enum
    {
        MainMenu,
        Game,
        Pause
    };
}

namespace xy
{
    class ShaderResource;
}

struct KeyMap final
{
    sf::Keyboard::Key up = sf::Keyboard::W;
    sf::Keyboard::Key down = sf::Keyboard::S;
    sf::Keyboard::Key left = sf::Keyboard::A;
    sf::Keyboard::Key right = sf::Keyboard::D;
    sf::Keyboard::Key fire = sf::Keyboard::Space;
    sf::Keyboard::Key pickup = sf::Keyboard::LControl;
    std::int32_t joyFire = 0;
    std::int32_t joyPickup = 1;
};

struct SharedData final
{
    xy::ResourceHandler resources;
    xy::ShaderResource* shaders = nullptr;
    enum
    {
        None,
        Paused,
        Died,
        GameOver,
        Error
    }pauseMessage = None;
    std::string messageString;

    enum
    {
        Win, Lose
    }gameoverType;

    enum
    {
        Hard = 1,
        Medium = 2,
        Easy = 3
    };
    std::int32_t difficulty = Easy;

    KeyMap keymap;

    LoadingScreen loadingScreen;
};