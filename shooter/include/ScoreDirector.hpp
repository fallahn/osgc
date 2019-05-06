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

namespace sf
{
    class Font;
}

struct SharedData;
class ScoreDirector final : public xy::Director
{
public:
    ScoreDirector(sf::Font&, std::int32_t, SharedData&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

private:

    sf::Font& m_font;
    SharedData& m_sharedData;

    std::int32_t m_alienCount;
    std::int32_t m_humanCount;

    sf::Clock m_alienClock;
    sf::Clock m_humanClock;

    void spawnScoreItem(sf::Vector2f, std::int32_t);
    void updateAlienCount();
    void updateHumanCount();
};