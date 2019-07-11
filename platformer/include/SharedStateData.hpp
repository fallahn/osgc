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

#include "InputBinding.hpp"
#include "StateIDs.hpp"

#include <SFML/Graphics/Texture.hpp>

namespace sf
{
    class Shader;
}

namespace MenuID
{
    enum
    {
        NewGame = 0, Continue, GameOver, Options, Quit
    };
};

struct TransitionContext final
{
    sf::Texture texture;
    sf::Shader* shader = nullptr;
    float duration = 1.f;
    std::int32_t nextState = StateID::Game;
};

struct Inventory final
{
    static constexpr std::int32_t MaxAmmo = 10;
    static constexpr std::int32_t MaxCoins = 999;
    static constexpr std::int32_t MaxLives = 99;

    std::int32_t lives = 3;
    std::int32_t coins = 0;
    std::int32_t score = 0;
    std::int32_t ammo = MaxAmmo;
};

struct SharedData final
{
    InputBinding inputBinding;
    std::int32_t menuID = -1;
    std::string nextMap = "gb01.tmx";
    std::string theme = "gearboy";
    Inventory inventory;

    TransitionContext transitionContext;
    std::string dialogueFile = "intro.txt";

    void saveProgress();
    void loadProgress();
    void reset();
};
