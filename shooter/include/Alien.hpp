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

struct Alien final
{
    sf::Vector2f velocity;
    enum class Type
    {
        Beetle, Scorpion
    }type = Type::Beetle;

    static constexpr std::int32_t NumberPerRound = 100;
};

class AlienSystem final : public xy::System
{
public:
    AlienSystem(xy::MessageBus&, const SpriteArray&, std::int32_t difficulty);

    void process(float) override;

    void addSpawn(sf::Vector2f);
    void clearSpawns();

private:
    const SpriteArray& m_sprites;

    std::vector<sf::Vector2f> m_spawnPoints;
    std::size_t m_spawnIndex;
    std::size_t m_spawnCount;
    sf::Clock m_spawnClock;

    std::int32_t m_alienCount;
    std::int32_t m_difficulty;

    void spawnAlien();

    struct Results final
    {
        sf::Vector2f coalescence;
        sf::Vector2f alignment;
        sf::Vector2f separation;
        sf::Vector2f humanAttraction;
    }; //allows sharing particular tree queries but adding the results in the correct order.

    void processBoid(xy::Entity, Results&);
    void solidCollision(xy::Entity);

    xy::Entity m_debug;
};