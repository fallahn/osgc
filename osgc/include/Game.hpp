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

#pragma once

#include "States.hpp"
#include <xyginext/core/App.hpp>
#include <any>

using SharedStateData = std::any;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else

#endif

class Game final : public xy::App
{
public:
    Game();
    ~Game();
    Game(const Game&) = delete;
    Game& operator = (const Game&) = delete;

    void loadPlugin(const std::string&);
    void unloadPlugin();

private:

    xy::StateStack m_stateStack;
    SharedStateData m_sharedData;

    std::string m_rootPath;

    void handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;

    void registerStates() override;
    void updateApp(float dt) override;
    void draw() override;

    bool initialise() override;
    void finalise() override;

#ifdef _WIN32
    HINSTANCE m_pluginHandle = nullptr;
#else
    void* m_pluginHandle = nullptr;
#endif //_win32
};
