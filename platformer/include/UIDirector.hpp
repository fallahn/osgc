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

#include <xyginext/ecs/Director.hpp>

#include <SFML/System/Clock.hpp>

struct SharedData;

namespace sf
{
    class Font;
}

class UIDirector final : public xy::Director
{
public:
    UIDirector(SharedData&, const sf::Font&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

private:

    SharedData& m_sharedData;
    const sf::Font& m_font;

    sf::Clock m_roundClock;
    bool m_dialogueShown;

    void updateTimer();
    void updateScore();
    void updateLives();
    void updateAmmo();

    void spawnWarning();
};