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

#include "Collision.hpp"

#include <SFML/Graphics/Texture.hpp>

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

using Path = std::pair<sf::Vector2f, sf::Vector2f>;
using EnemySpawn = std::pair<Path, std::int32_t>;

struct MapLayer final
{
    sf::Texture* indexMap = nullptr;
    sf::Texture* tileSet = nullptr;
    // x/y count of tiles in the tile set (not pixels)
    sf::Vector2f tileSetSize; 
    //the world size of the layer. This is pre-pixel scaling.
    //divide by the indexMap size to get the tile size
    sf::Vector2f layerSize; 
};

class MapLoader final
{
public:
    MapLoader();

    bool load(const std::string& map);

    const std::vector<MapLayer>& getLayers() const { return m_mapLayers; }

    float getTileSize() const { return m_tileSize; }

    const std::vector<CollisionShape>& getCollisionShapes() const { return m_collisionShapes; }

    sf::Vector2f getMapSize() const { return m_mapSize; }

    sf::Vector2f getSpawnPoint() const { return m_spawnPoint; }

    const std::string& getNextMap() const { return m_nextMap; }

    const std::string getNextTheme() const { return m_nextTheme; }

    const std::vector<EnemySpawn>& getEnemySpawns() const { return m_enemySpawns; }

    const std::vector<std::string>& getDialogueFiles() const { return m_dialogueFiles; }

    const std::vector<std::pair<std::int32_t, sf::FloatRect>>& getProps() const { return m_props; }

private:

    //mapped to strings so we don't load the same texture more than once
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_tilesets;

    //used for tile lookup indices. There are in the same order as
    //the layer vector as each layer has one lookup texture.
    std::vector<std::unique_ptr<sf::Texture>> m_indexMaps;

    //holds all the info needed to feed the layer drawable component
    //this is in bottom up order.
    std::vector<MapLayer> m_mapLayers;

    std::vector<std::pair<std::int32_t, sf::FloatRect>> m_props;

    float m_tileSize;
    sf::Vector2f m_mapSize;
    sf::Vector2f m_spawnPoint;
    std::string m_nextMap;
    std::string m_nextTheme;

    std::vector<CollisionShape> m_collisionShapes;
    std::vector<EnemySpawn> m_enemySpawns;
    std::vector<std::string> m_dialogueFiles;
};