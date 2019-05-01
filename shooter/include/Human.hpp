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

#include "ResourceIDs.hpp"

#include <xyginext/ecs/System.hpp>

#include <SFML/System/Clock.hpp>

#include <vector>

struct Human final
{
    enum class State
    {
        Normal,
        Seeking,
        Scared
    }state = State::Seeking;

    sf::Vector2f velocity;
    float speed = 120.f;
};

class HumanSystem final : public xy::System
{
public:
    HumanSystem(xy::MessageBus&, const SpriteArray&);

    void process(float) override;

    void addSpawn(sf::Vector2f v) { m_spawnPoints.push_back(v); }

    void clearSpawns();

private:
    const SpriteArray& m_sprites;
    std::vector<sf::Vector2f> m_spawnPoints;
    std::size_t m_spawnIndex;
    std::size_t m_spawnCount;

    sf::Clock m_spawnClock;

    void spawnHuman();
    void updateNormal(xy::Entity);
    void updateSeeking(xy::Entity);
    void updateScared(xy::Entity);

    void updateCollision(xy::Entity);
};