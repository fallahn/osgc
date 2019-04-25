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

#include <xyginext/ecs/Director.hpp>

#include <SFML/System/Clock.hpp>

#include <array>

class SpawnDirector final : public xy::Director
{
public:
    explicit SpawnDirector(SpriteArray&);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:

    SpriteArray& m_sprites;

    enum ActiveItem
    {
        Ammo, Battery, 

        Count
    };
    std::array<std::size_t, ActiveItem::Count> m_activeItems = {};
    std::array<sf::Clock, ActiveItem::Count> m_itemClocks;

    void spawnExplosion(sf::Vector2f);
    void spawnMiniExplosion(sf::Vector2f);
    void spawnAmmo(sf::Vector2f);
    void spawnBattery(sf::Vector2f);

    sf::Vector2f getRandomPosition();
};