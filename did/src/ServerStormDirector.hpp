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
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace Server
{
    struct SharedStateData;
}

class StormDirector final : public xy::Director
{
public:
    explicit StormDirector(Server::SharedStateData&);

    void handleMessage(const xy::Message&) override;
    void handleEvent(const sf::Event&) override{}

    void process(float) override;

    enum State
    {
        Dry = 0,
        Wet,
        Stormy
    };
private:
    Server::SharedStateData& m_sharedData;
    std::vector<sf::Vector2f> m_strikePoints;

    std::size_t m_strikePointIndex;
    std::size_t m_strikeTimeIndex;
    sf::Clock m_strikeClock;

    State m_state = Dry;

    sf::Clock m_stateClock;
    std::size_t m_stateIndex;

    void setWeather(State);
};
