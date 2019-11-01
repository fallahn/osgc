/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "PluginExport.hpp"
#include "StateIDs.hpp"
#include "MainState.hpp"
#include "PauseState.hpp"
#include "AttractState.hpp"
#include "GameOverState.hpp"
#include "IniParse.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>
#include <xyginext/core/Assert.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/graphics/BitmapFont.hpp>

int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
    XY_ASSERT(ss, "State stack is nullptr");

    *sharedData = std::make_any<SharedData>();
    auto& sd = std::any_cast<SharedData&>(*sharedData);

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/misc.spt", sd.resources);
    sd.sprites[SpriteID::GameOver] = spriteSheet.getSprite("game_over");
    sd.sprites[SpriteID::Pause] = spriteSheet.getSprite("paused");
    sd.sprites[SpriteID::Title] = spriteSheet.getSprite("title");
    sd.sprites[SpriteID::TopBar] = spriteSheet.getSprite("top_bar");
    sd.sprites[SpriteID::NameInput] = spriteSheet.getSprite("name_box");

    sd.fontID = sd.resources.load<xy::BitmapFont>("assets/images/bmp_font.png");

    ss->registerState<MainState>(StateID::Main, sd);
    ss->registerState<PauseState>(StateID::Pause, sd);
    ss->registerState<AttractState>(StateID::Attract, sd);
    ss->registerState<GameOverState>(StateID::GameOver, sd);

    IniParse ini;
    const std::string section("scores");
    if (ini.parse(xy::FileSystem::getConfigDirectory(xy::App::getActiveInstance()->getApplicationName()) + "scores.ini"))
    {
        auto& data = ini.getData();
        if (data.count(section) != 0)
        {
            std::size_t i = 0;
            for (const auto& [name, value] : data[section])
            {
                sd.highScores[i++] = std::make_pair(name, ini.getValueInt(section, name));
                if (i == sd.highScores.size())
                {
                    break;
                }
            }
        }

        std::sort(sd.highScores.begin(), sd.highScores.end(),
            [](const std::pair<std::string, std::int32_t>& a, const std::pair<std::string, std::int32_t>& b)
            {
                return a.second > b.second;
            });
    }

    xy::App::getActiveInstance()->setWindowTitle("Bubble Puzzle 2097");

    return StateID::Main;
}

void end(xy::StateStack* ss)
{
    ss->unregisterState(StateID::GameOver);
    ss->unregisterState(StateID::Attract);
    ss->unregisterState(StateID::Pause);
    ss->unregisterState(StateID::Main);
}