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

#include "GlobalConsts.hpp"
#include "FoliageGenerator.hpp"

#include <SFML/Graphics/RenderTexture.hpp>

#include <array>
#include <vector>

namespace xy
{
    class TextureResource;
}

struct TileArray final
{
    std::uint8_t data[Global::TileCount] = {};

    //cos I'm too lazy to refactor all appearances
    std::uint8_t& operator [] (std::size_t idx)
    {
        return data[idx];
    }

    const uint8_t& operator [] (std::size_t idx) const
    {
        return data[idx];
    }
};

struct MapData final
{
    TileArray tileData;
    std::array<FoliageData, Global::MaxFoliage> foliageData = {};
    std::size_t foliageCount = 0;
};

struct ActorSpawn final
{
    ActorSpawn() = default;
    ActorSpawn(float x, float y, std::int32_t i) : position(x, y), id(i) {}
    sf::Vector2f position;
    std::int32_t id = 0;
};

class IslandGenerator final
{
public:
    IslandGenerator();
 
    void generate(int);

    void render(const TileArray&, xy::TextureResource&);

    const sf::Texture& getTexture() const;
    const sf::Texture& getNormalMap() const;
    const sf::Texture& getWaveMap() const;

    const MapData& getMapData() const { return m_mapData; }

    const std::vector<ActorSpawn>& getActorSpawns() const { return m_actorSpawns; }
    std::size_t getTreasureCount() const { return m_treasureCount; }

    //used to get a better set of tile data for path finding
    //returns *post* edge processing
    const TileArray& getPathData();

    //returns if the given position is in water - depends on map data already being processed
    bool isWater(sf::Vector2f) const;

private:

    bool m_edgesProcessed;

    void heightNoise(int);
    void edgeNoise(int);
    void processFoliage();
    void processEdges();

    MapData m_mapData;
    sf::RenderTexture m_renderTexture;
    sf::RenderTexture m_normalMapTexture;
    sf::RenderTexture m_waveTexture;

    std::vector<ActorSpawn> m_actorSpawns;
    std::size_t m_treasureCount;
};