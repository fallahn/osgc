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

#include <SFML/Graphics/RenderTexture.hpp>

#include <string>
#include <vector>

struct CollisionBox;
class MapLoader final
{
public:
    explicit MapLoader(const SpriteArray&);

    bool load(const std::string&);

    const sf::Texture& getTopDownTexture() const;

    const sf::Texture& getSideTexture() const;

    void renderSprite(std::int32_t, sf::Vector2f, float = 0.f);

    const std::vector<CollisionBox>& getCollisionBoxes() const;

    const std::vector<sf::Vector2f>& getNavigationNodes() const;

    enum SpawnType
    {
        Human, Alien
    };
    const std::vector<std::pair<sf::Vector2f, std::int32_t>>& getSpawnPoints() const;

private:

    const SpriteArray& m_sprites;

    sf::RenderTexture m_topBuffer;
    sf::RenderTexture m_topTexture;
    sf::RenderTexture m_sideTexture;

    std::vector<CollisionBox> m_collisionBoxes;
    std::vector<sf::Vector2f> m_navigationNodes;
    std::vector<std::pair<sf::Vector2f, std::int32_t>> m_spawnPoints;
};