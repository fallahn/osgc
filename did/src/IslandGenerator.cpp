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

#include "IslandGenerator.hpp"
#include "glad/glad.h"
#include "fastnoise/FastNoiseSIMD.h"
#include "ServerRandom.hpp"
#include "Actor.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/resources/Resource.hpp>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

using fn = FastNoiseSIMD;

namespace
{
    const sf::Vector2u NormalMapSize(Global::IslandSize/* / 2.f*/);
    const float OutlinePixelOffset = 1.f / (Global::IslandSize.x /*/ 4.f*/);

    enum TileType
    {
        DarkSand = 2,
        LightSand = 4,
        Grass = 6
    };

    const float BoatRadSqr = (Global::BoatRadius * 3.5f) * (Global::BoatRadius * 3.5f);

    //converts the rendered heightmap to a normal map
const std::string& NormalFrag =
R"(
#version 120
            
uniform sampler2D u_texture;
uniform float u_textureSize;

const float strength = 0.65;

void main()
{
    float pixelSize = 1.0 / u_textureSize;
    vec2 uv = gl_TexCoord[0].xy;

    //sample surrounding pixels
    float tl = texture2D(u_texture, uv + (pixelSize * vec2(-1.0, -1.0))).r;
    float l = texture2D(u_texture, uv + (pixelSize * vec2(-1.0, 0.0))).r;
    float bl = texture2D(u_texture, uv + (pixelSize * vec2(-1.0, 1.0))).r;
    float t = texture2D(u_texture, uv + (pixelSize * vec2(0.0, -1.0))).r;
    float b = texture2D(u_texture, uv + (pixelSize * vec2(0.0, 1.0))).r;
    float tr = texture2D(u_texture, uv + (pixelSize * vec2(1.0, -1.0))).r;
    float r = texture2D(u_texture, uv + (pixelSize * vec2(1.0, 0.0))).r;
    float br = texture2D(u_texture, uv + (pixelSize * vec2(1.0, 1.0))).r;

    //sobel
    // -1 0 1
    // -2 0 2
    // -1 0 1
    float dx = tr + (2.0 * r) + br - tl - (2.0 * l) - bl;

    // -1 -2 -1
    //  0  0  0
    //  1  2  1
    float dy = tl + (2.0 * t) + tr - bl - (2.0 * b) - br;

    vec3 normal = normalize(vec3(-dx * strength, -dy * strength, 1.0));

    gl_FragColor = vec4(normal * 0.5 + 0.5, 1.0);
})";

//draws outline used in wave texture
const std::string OutlineFrag =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_pixelOffset;

const float Thickness = 4.0;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    float baseAlpha = texture2D(u_texture, uv).a;
    float alpha = Thickness * 4.0 * baseAlpha;

    for(float i = 1.0; i <= Thickness; ++i)
    {
        alpha -= texture2D(u_texture, uv - vec2(u_pixelOffset * i, 0.0f)).a;
        alpha -= texture2D(u_texture, uv - vec2(-u_pixelOffset * i, 0.0f)).a;
        alpha -= texture2D(u_texture, uv - vec2(0.0f, u_pixelOffset * i)).a;
        alpha -= texture2D(u_texture, uv - vec2(0.0f, -u_pixelOffset * i)).a;
    }

    float wave = (sin(uv.x * 100.0) + 1.0 / 2.0);
    wave *= (cos(uv.y * 100.0) + 1.0 / 2.0);
    wave = clamp(wave + 0.8, 0.0, 1.0);

    vec3 waveColour = vec3(1.0) * clamp(alpha, 0.0, 1.0) * 0.9 * wave;

    gl_FragColor = vec4(waveColour, waveColour.r);// clamp((1.0 - baseAlpha) + (waveColour.b * 0.9999), 0.0, 1.0));
})";
}

IslandGenerator::IslandGenerator()
    : m_treasureCount   (0),
    m_edgesProcessed    (false)
{

}

//public
void IslandGenerator::generate(int seed)
{
    edgeNoise(seed);

    static const int beeChance = 140;
    static const int birdChance = 60;
    for (auto i = 0u; i < Global::TileCount; ++i)
    {
        //spawn bees
        if (m_mapData.tileData[i] >= TileType::Grass)
        {
            if (Server::getRandomInt(0, beeChance) == 0)
            {
                auto x = i % Global::TileCountX;
                auto y = i / Global::TileCountX;
                m_actorSpawns.emplace_back(x * Global::TileSize, (y * Global::TileSize) + 6.f, Actor::Bees);

                continue; //no chance of spawning birds on top
            }

            if (Server::getRandomInt(0, birdChance) == 0)
            {
                auto x = i % Global::TileCountX;
                auto y = i / Global::TileCountX;
                m_actorSpawns.emplace_back(x * Global::TileSize, (y * Global::TileSize) - 2.f, Actor::Parrot);
            }
        }
    }

    //chose one of 3 edges for a seagull
    auto edge = Server::getRandomInt(0, 2);
    int x = 0;
    int y = 0;
    switch (edge)
    {
    default:
    case 0:
    {
        y = Server::getRandomInt(0, Global::TileCountY-6);
        m_actorSpawns.emplace_back(-64.f, y * Global::TileSize, Actor::Seagull);
    }
        break;
    case 1:
    {
        x = Server::getRandomInt(0, Global::TileCountX);
        m_actorSpawns.emplace_back(x * Global::TileSize, -64.f, Actor::Seagull);
    }
        break;
    case 2:
    {
        y = Server::getRandomInt(0, Global::TileCountY-6);
        m_actorSpawns.emplace_back(Global::IslandSize.y + 64.f, y * Global::TileSize, Actor::Seagull);
    }
        break;
    }

    //throw in some rocks etc
    auto rockCount = Server::getRandomInt(1, 4);
    int startTile = 0;
    for (auto i = 0; i < rockCount; ++i)
    {
        auto rockY = Server::getRandomInt(startTile, Global::TileCountY - 10);
        startTile = std::min(rockY + 1, static_cast<int>(Global::TileCountY - 11));
        if (rockY != y) //don't overlap with seagull
        {
            m_actorSpawns.emplace_back(-48.f + Server::getRandomFloat(-12.f, 12.f), rockY * Global::TileSize, Actor::WaterDetail);
        }
    }
    rockCount = Server::getRandomInt(1, 4);
    startTile = 2;
    for (auto i = 0; i < rockCount; ++i)
    {
        auto rockY = Server::getRandomInt(startTile, Global::TileCountY - 10);
        startTile = std::min(rockY + 1, static_cast<int>(Global::TileCountY - 11));
        if (rockY != y) //don't overlap with seagull
        {
            m_actorSpawns.emplace_back(Global::IslandSize.y + 48.f + Server::getRandomFloat(-12.f, 12.f), rockY * Global::TileSize, Actor::WaterDetail);
        }
    }
    rockCount = Server::getRandomInt(1, 4);
    startTile = 4;
    for (auto i = 0; i < rockCount; ++i)
    {
        auto rockX = Server::getRandomInt(startTile, Global::TileCountX);
        startTile = std::min(rockX + 1, static_cast<int>(Global::TileCountX - 1));
        if (rockX != x) //don't overlap with seagull
        {
            m_actorSpawns.emplace_back(rockX * Global::TileSize, -48.f + Server::getRandomFloat(-12.f, 12.f), Actor::WaterDetail);
        }
    }


    //makes sure any spawn coordinates are on a valid tile
    auto validateSpawn = [&](int& x0, int& y0) //x0,y0 to prevent accidentally capturing x,y above
    {
        //check if it's on solid then move in a random direction until it isn't
        int up = Server::getRandomInt(0, 1) == 0 ? -2 : 2;
        while (m_mapData.tileData[y0 * Global::TileCountX + x0] >= TileType::Grass)
        {
            y += up;
        }

        if (m_mapData.tileData[(y0 + 1) * Global::TileCountX + x0] >= TileType::Grass)
        {
            y--;
        }
        else if (m_mapData.tileData[(y0 - 1) * Global::TileCountX + x0] >= TileType::Grass)
        {
            y++;
        }

        //we don't want to dig in the gaps between hedges - so move if grass either side
        if (m_mapData.tileData[y0 * Global::TileCountX + (x0 - 2)] >= TileType::Grass &&
            m_mapData.tileData[y0 * Global::TileCountX + (x0 + 2)] >= TileType::Grass)
        {
            y0 += up;
        }

        //check we haven't landed on a patch of dark sand
        if (m_mapData.tileData[y0 * Global::TileCountX + x0] < TileType::LightSand)
        {
            if (auto tile = m_mapData.tileData[y0 * Global::TileCountX + (x0 + 2)];
                tile >= TileType::LightSand && tile < TileType::Grass/*tile < TileType::DarkSand*/)
            {
                x0 += 2;
                
            }
            else if (auto tile = m_mapData.tileData[y0 * Global::TileCountX + (x0 - 2)];
                tile >= TileType::LightSand && tile < TileType::Grass/*tile < TileType::DarkSand*/)
            {
                x0 -= 2;
            }
            else if (auto tile = m_mapData.tileData[(y0 + 2) * Global::TileCountX + x0];
                tile >= TileType::LightSand && tile < TileType::Grass)
            {
                y0 += 2;
            }
            else if (auto tile = m_mapData.tileData[(y0 - 2) * Global::TileCountX + x0];
                tile >= TileType::LightSand && tile < TileType::Grass)
            {
                y0 -= 2;
            }
        }
    };

    //spawn min chest count
    sf::Vector2f treasurePos;
    //for (auto i = 0; i < 2; ++i)
    {
        //pick a point at random within 4 tiles from the edge
        x = Server::getRandomInt(4, Global::TileCountX - 8);
        y = Server::getRandomInt(4, Global::TileCountY - 8);

        validateSpawn(x, y);

        treasurePos = { x * Global::TileSize, y * Global::TileSize };
        m_actorSpawns.emplace_back(treasurePos.x, treasurePos.y, Actor::TreasureSpawn);
        m_treasureCount++;
    }

    //use poisson disc to spawn ammo and skeletons
    //there's a bug in the generator which means we can't offset the destination area...
    sf::Vector2f offset(6.f , 6.f); //so we'll add it ourself
    auto points = xy::Util::Random::poissonDiscDistribution(
        { 0.f, 0.f, static_cast<float>(Global::TileCountX -  12.f), static_cast<float>( Global::TileCountY - 12.f) },
        8.f, 15, Server::rndEngine);

    const float minTreaureDistance = (8.f * Global::TileSize) * (8.f * Global::TileSize);
    for (auto point : points)
    {
        point += offset;
        x = static_cast<int>(point.x);
        y = static_cast<int>(point.y);

        validateSpawn(x, y);

        //only place if not too close to treasure spawn
        //or boat/player spawn
        sf::Vector2f spawnPos(x * Global::TileSize, y * Global::TileSize);
        bool canPlace = (xy::Util::Vector::lengthSquared(spawnPos - treasurePos) > minTreaureDistance);

        if (canPlace)
        {
            for (auto boatPoint : Global::BoatPositions)
            {
                auto dist = xy::Util::Vector::lengthSquared(spawnPos - boatPoint);
                canPlace = (dist > BoatRadSqr);
                if (!canPlace)
                {
                    break;
                }
            }
        }
        
        if (canPlace)
        {
            m_actorSpawns.emplace_back(spawnPos.x, spawnPos.y, Server::getRandomInt(Actor::AmmoSpawn, Actor::CoinSpawn));
        }
    }

    std::shuffle(m_actorSpawns.begin(), m_actorSpawns.end(), Server::rndEngine);

    //convert a few more to treasure
    auto stride = m_actorSpawns.size() / 7; //max extra treasures - we want 5 in total so there should always be at least one winner
    for (auto i = stride; i < m_actorSpawns.size(); i += stride)
    {
        if ((m_actorSpawns[i].id == Actor::AmmoSpawn
            || m_actorSpawns[i].id == Actor::CoinSpawn)
            && m_treasureCount < 5)
        {
            m_actorSpawns[i].id = Actor::TreasureSpawn;
            m_treasureCount++;
        }
    }

    //make sure we have a min count of 3
    if (m_treasureCount < 3)
    {
        auto result = std::find_if(m_actorSpawns.begin(), m_actorSpawns.end(),
            [](const ActorSpawn& s) {return s.id == Actor::AmmoSpawn || s.id == Actor::CoinSpawn; });

        if (result != m_actorSpawns.end())
        {
            result->id = Actor::TreasureSpawn;
            m_treasureCount++;
        }
    }
}

void IslandGenerator::render(const TileArray& tileData, xy::TextureResource& tr)
{
    m_mapData.tileData = tileData;    
  
    processFoliage();

    //marching square pass
    processEdges();

    if (!m_renderTexture.create(static_cast<int>(Global::IslandSize.x), static_cast<int>(Global::IslandSize.y)))
    {
        xy::Logger::log("Failed creating render texture for island generator", xy::Logger::Type::Error);
    }

    if (!m_normalMapTexture.create(NormalMapSize.x, NormalMapSize.y))
    {
        xy::Logger::log("Failed creating normal map render texture for island generator", xy::Logger::Type::Error);
    }

    if (!m_waveTexture.create(NormalMapSize.x/* / 4*/, NormalMapSize.y /*/ 4*/))
    {
        xy::Logger::log("Failed creating wave render texture for island generator", xy::Logger::Type::Error);
    }   
    
    auto details = xy::Util::Random::poissonDiscDistribution(
        sf::FloatRect(sf::Vector2f(), Global::IslandSize),
        150.f, 50);

    sf::Texture& tileset = tr.get("assets/images/test_set.png");
    sf::Texture& detailset = tr.get("assets/images/details.png");
    sf::Texture& heightset = tr.get("assets/images/test_set_height.png");

    sf::Shader normalShader;
    normalShader.loadFromMemory(NormalFrag, sf::Shader::Fragment);
    normalShader.setUniform("u_textureSize", static_cast<float>(NormalMapSize.x));
    
    /*if (tileset.loadFromFile("assets/images/test_set.png")
        && detailset.loadFromFile("assets/images/details.png")
        && heightset.loadFromFile("assets/images/test_set_height.png"))*/
    {
        int tileSetCountX = tileset.getSize().x / static_cast<int>(Global::TileSize);
        int tileSetCountY = tileset.getSize().y / static_cast<int>(Global::TileSize);

        sf::Sprite sprite(heightset);
        //sprite.setScale(0.5f, 0.5f); //remember to reset this if changing buffer size

        auto getSubrect = [&tileSetCountX, &tileSetCountY](std::uint8_t idx) ->sf::IntRect
        {
            int left = (idx % tileSetCountX) * static_cast<int>(Global::TileSize);
            int top = (idx / tileSetCountX) * static_cast<int>(Global::TileSize);
            return { left, top, static_cast<int>(Global::TileSize), static_cast<int>(Global::TileSize) };
        };

        //normal map
        sf::RenderTexture tempTexture;
        tempTexture.create(NormalMapSize.x, NormalMapSize.y);
        tempTexture.clear(sf::Color::Blue);
        for (auto i = 0u; i < Global::TileCount; ++i)
        {
            sprite.setTextureRect(getSubrect(m_mapData.tileData[i]));

            float posX = ((i % Global::TileCountX) * Global::TileSize);// / 2.f;
            float posY = ((i / Global::TileCountX) * Global::TileSize);// / 2.f;
            sprite.setPosition(posX, posY);

            tempTexture.draw(sprite);
        }
        tempTexture.display();
        //render to buffer with normal map shader
        sprite.setTexture(tempTexture.getTexture(), true);
        sprite.setPosition(0.f, 0.f);
        //sprite.setScale(1.f, 1.f);
        normalShader.setUniform("u_texture", tempTexture.getTexture());
        m_normalMapTexture.clear(sf::Color::Red);
        m_normalMapTexture.draw(sprite, &normalShader);
        m_normalMapTexture.display();

        //colour map
        //sf::RectangleShape shape({ Global::TileSize, Global::TileSize });
        sprite.setTexture(tileset);
        m_renderTexture.clear(sf::Color::Transparent);
        for (auto i = 0u; i < Global::TileCount; ++i)
        {
            sprite.setTextureRect(getSubrect(m_mapData.tileData[i]));

            float posX = (i % Global::TileCountX) * Global::TileSize;
            float posY = (i / Global::TileCountX) * Global::TileSize;
            sprite.setPosition(posX, posY);

            m_renderTexture.draw(sprite);

            /*if (tileData[i] < TileType::DarkSand)
            {
                shape.setFillColor({ 0, 0, 120, 120 });
            }
            else if (tileData[i] < TileType::Grass)
            {
                shape.setFillColor({ 0, 120, 0, 120 });
            }
            else
            {
                shape.setFillColor({ 120, 0, 0, 120 });
            }
            shape.setPosition(posX - (Global::TileSize / 2.f), posY - (Global::TileSize / 2.f));
            m_renderTexture.draw(shape);*/
        }

        //sprinkle details
        tileSetCountX = detailset.getSize().x / Global::TileSize;
        tileSetCountY = detailset.getSize().y / Global::TileSize;
        auto tileSetCount = tileSetCountX * tileSetCountY;

        sprite.setTexture(detailset);

        for (auto pos : details)
        {
            //check we're not in the water
            auto tileX = std::floor(pos.x / Global::TileSize);
            auto tileY = std::floor(pos.y / Global::TileSize);
            int idx = static_cast<int>(tileY) * Global::TileCountX + tileX;
            
            if (m_mapData.tileData[idx] == 60 || m_mapData.tileData[idx] == 75)
            {
                sprite.setTextureRect(getSubrect(xy::Util::Random::value(0, tileSetCount - 1)));
                sprite.setPosition(pos);
                m_renderTexture.draw(sprite);
            }
        }

        m_renderTexture.display();

        //take colour map and render outline with it for wave effect
        sf::Shader outlineShader;
        outlineShader.loadFromMemory(OutlineFrag, sf::Shader::Fragment);
        outlineShader.setUniform("u_texture", m_renderTexture.getTexture());
        outlineShader.setUniform("u_pixelOffset", OutlinePixelOffset);

        sprite.setTexture(m_renderTexture.getTexture(), true);
        sprite.setPosition(0.f, 0.f);
        sprite.setScale(1.f, 1.f);
        m_waveTexture.clear(sf::Color::Transparent);
        m_waveTexture.draw(sprite, &outlineShader);
        m_waveTexture.display();
    }
    /*else
    {
        xy::Logger::log("failed loading island, missing texture", xy::Logger::Type::Error);
    }*/
}

const sf::Texture& IslandGenerator::getTexture() const
{
    return m_renderTexture.getTexture();
}

const sf::Texture& IslandGenerator::getNormalMap() const
{
    return m_normalMapTexture.getTexture();
}

const sf::Texture& IslandGenerator::getWaveMap() const
{
    return m_waveTexture.getTexture();
}

const TileArray& IslandGenerator::getPathData()
{
    if (!m_edgesProcessed)
    {
        processEdges();
    }
    return m_mapData.tileData;
}

bool IslandGenerator::isWater(sf::Vector2f position) const
{
    position.x += Global::TileSize / 2.f;
    position.y += Global::TileSize / 2.f;

    sf::Vector2i tileIndex(position / Global::TileSize);
    auto idx = tileIndex.y * Global::TileCountX + tileIndex.x;
    return (m_mapData.tileData[idx] == 0 || m_mapData.tileData[idx] == 15);
}

bool IslandGenerator::isEdgeTile(sf::Vector2f position) const
{
    position.x += Global::TileSize / 2.f;
    position.y += Global::TileSize / 2.f;

    sf::Vector2i tileIndex(position / Global::TileSize);
    auto idx = tileIndex.y * Global::TileCountX + tileIndex.x;
    return (m_mapData.tileData[idx] < 30);
}

//private
void IslandGenerator::heightNoise(int seed)
{
    //attempts to create an island based on height data
    //results... not amazing

    auto noise = fn::NewFastNoiseSIMD(seed);
    noise->SetFractalOctaves(4);
    //noise->SetFractalLacunarity(10);
    //noise->SetFrequency(0.02);

    auto landNoise = noise->GetSimplexFractalSet(0, 0, 0, Global::TileCountX, Global::TileCountY, 1);

    auto smoothStep = [](float edge0, float edge1, float x)->float
    {
        float t = xy::Util::Math::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    };

    const float threshold = 0.25f;
    const sf::Vector2f centre(Global::TileCountX / 2, Global::TileCountY / 2);
    for (auto i = 0u; i < Global::TileCount; ++i)
    {
        auto val = landNoise[i];

        //calc dist from centre and fade out
        float x = static_cast<float>(i % Global::TileCountX);
        float y = static_cast<float>(i / Global::TileCountY);
        //float distance = xy::Util::Vector::length(sf::Vector2f(x, y) - centre);
        float smooth = smoothStep(22.f, 15.f, std::abs(x - centre.x)) * smoothStep(22.f, 15.f, std::abs(y - centre.y));
        val *= smooth;

        val = (val > threshold) ? 1.f : 0.f;

        m_mapData.tileData.data[i] = static_cast<std::uint8_t>(val * 255.f);
    }
    fn::FreeNoiseSet(landNoise);
}

void IslandGenerator::edgeNoise(int seed)
{
    //creates a 1D noise used to shape the edges of the island
    auto noise = fn::NewFastNoiseSIMD(seed);
    noise->SetFractalOctaves(1);
    noise->SetFrequency(0.05f);

    auto landNoise = noise->GetSimplexFractalSet(0, 0, 0, (Global::TileCountX * 2) + ((Global::TileCountY * 2) - 4), 1, 1);

    //as there are 2 rows in the tileset for variations
    //of the same contour, material types change every *2* values
    //ie 0 -1 is water, 2-3 sand and so on.
    for (auto i = 0u; i < Global::TileCount; ++i)
    {
        m_mapData.tileData.data[i] = TileType::LightSand;
        m_mapData.tileData.data[i] += Server::getRandomInt(0, 1);
    }

    //lookup func
    auto getVal = [landNoise](std::size_t i) -> int
    {
        return static_cast<int>(xy::Util::Math::round((landNoise[i] + 1.f / 2.f) * 2.f)) + 3;
    };

    //top row
    std::size_t currentIndex = 0;
    for (auto i = currentIndex; i < Global::TileCountX; ++i)
    {
        int val = getVal(i);

        for (auto y = 0; y < val; ++y)
        {
            m_mapData.tileData.data[i + (y * Global::TileCountX)] = 0;
        }

        //add two rows of dark sand
        for (auto y = val; y < val + 2; ++y)
        {
            m_mapData.tileData.data[i + (y * Global::TileCountX)] = TileType::DarkSand + Server::getRandomInt(0, 1);
        }
    
    }
    currentIndex = Global::TileCountX;

    //right side
    for (auto i = currentIndex; i < (Global::TileCountX + (Global::TileCountY - 1)); ++i)
    {
        int val = getVal(i);
        for (auto x = Global::TileCountX - 1; x >= Global::TileCountX - val; --x)
        {
            auto idx = ((i - Global::TileCountX) * Global::TileCountX) + x;
            m_mapData.tileData.data[idx] = 0;
        }

        auto start = Global::TileCountX - val;// -1;
        for (auto x = start; x >= start - 2; --x)
        {
            auto idx = ((i - Global::TileCountX) * Global::TileCountX) + x;
            if (m_mapData.tileData[idx] != 0)
            {
                m_mapData.tileData.data[idx] = TileType::DarkSand + Server::getRandomInt(0, 1);
            }
        }
    }
    currentIndex = (Global::TileCountX + (Global::TileCountY - 1));

    //bottom
    for (auto i = currentIndex; i < currentIndex + (Global::TileCountX - 1); ++i)
    {
        int val = getVal(i);
        int col = i - currentIndex;
        col = (Global::TileCount - 1) - col;

        for (auto y = 0; y < val; ++y)
        {
            auto idx = col - (Global::TileCountX * y);
            m_mapData.tileData.data[idx] = 0;
        }

        //add dark sand tiles
        for (auto y = val; y < val + 2; ++y)
        {
            auto idx = col - (Global::TileCountX * y);
            if (m_mapData.tileData[idx] != 0)
            {
                m_mapData.tileData.data[idx] = TileType::DarkSand + Server::getRandomInt(0, 1);
            }
        }
    }
    currentIndex += (Global::TileCountX - 1);

    //left side
    for (auto i = currentIndex; i < currentIndex + (Global::TileCountY - 2); ++i)
    {
        int val = getVal(i);
        int row = i - currentIndex;
        row = ((Global::TileCountY - 1) - row) * Global::TileCountX;

        for (auto x = row; x < row + val; ++x)
        {
            m_mapData.tileData.data[x] = 0;
        }

        auto start = row + val;
        for (auto x = start; x < start + 2; ++x)
        {
            if (m_mapData.tileData[x] != 0)
            {
                m_mapData.tileData.data[x] = TileType::DarkSand + Server::getRandomInt(0, 1);

                //if tile above or below is water add extra tile
                //if (m_mapData.tileData[x - Global::TileCountX] == 0
                //    /*|| m_mapData.tileData[x + Global::TileCountX] == 0*/)
                //{
                //    m_mapData.tileData[x - Global::TileCountX] = TileType::DarkSand + getRandInt(0, 1);
                //    std::cout << "buns\n";
                //}

                /*if (m_mapData.tileData[x + Global::TileCountX] <TileType::DarkSand)
                {
                    m_mapData.tileData[x + Global::TileCountX] = TileType::DarkSand + getRandInt(0, 1);
                    std::cout << "flaps\n";
                }*/
            }
        }
    }

    fn::FreeNoiseSet(landNoise);

    //create foliage rows
    static const std::size_t firstRow = Global::TileCountX * 9;
    static const std::size_t rowSpace = Global::TileCountX * 4;
    static const std::size_t rowCount = 8;

    int lastGap = 0;
    for (std::size_t i = firstRow, numRows = 0u; i < Global::TileCount && numRows < rowCount; i += rowSpace, ++numRows)
    {
        //skip at least 2 sand tiles on left
        auto j = i;
        auto edgeCount = 0;
        auto maxEdges = Server::getRandomInt(2, 4);
        while (edgeCount < maxEdges)
        {
            if (m_mapData.tileData[j] >= TileType::LightSand)
            {
                edgeCount++;
            }
            j++;
        }
        
        auto rowStart = j;
        auto rowLength = j;
        maxEdges = Server::getRandomInt(2, 4);

        //add grass until 1 from the edge of dark sand
        for (; j < i + Global::TileCountX; ++j)
        {
            if (m_mapData.tileData[j] >= TileType::LightSand && m_mapData.tileData[j+maxEdges] >= TileType::LightSand)
            {
                m_mapData.tileData.data[j] = Server::getRandomInt(TileType::Grass, TileType::Grass + 1);
                rowLength++;
            }
        }

        //add gaps
        auto gap = Server::getRandomInt(rowStart + 3, rowLength - 6);
        //try preventing gaps in similar positions across rows
        auto diff = std::abs(lastGap - (gap - static_cast<int>(i)));
        auto tries = 6;
        while (diff < 6 && tries--)
        {
            gap = Server::getRandomInt(rowStart + 3, rowLength - 6);
            diff = std::abs(lastGap - (gap - static_cast<int>(i)));
        }

        lastGap = gap - i;

        m_mapData.tileData.data[gap] = TileType::LightSand + Server::getRandomInt(0, 1);;
        m_mapData.tileData.data[gap + 1] = TileType::LightSand + Server::getRandomInt(0, 1);;

        //add extra gap if gap near one ed or other
        auto relLength = rowLength - i;
        if (lastGap < (relLength / 4))
        {
            auto offset = relLength / 3;
            m_mapData.tileData.data[gap + offset] = TileType::LightSand + Server::getRandomInt(0, 1);;
            m_mapData.tileData.data[gap + offset + 1] = TileType::LightSand + Server::getRandomInt(0, 1);

            lastGap = (gap + offset) - i;
        }
        else if (lastGap > ((relLength / 4) * 32))
        {
            auto offset = relLength / 3;
            m_mapData.tileData.data[gap - offset] = TileType::LightSand + Server::getRandomInt(0, 1);;
            m_mapData.tileData.data[(gap - offset) + 1] = TileType::LightSand + Server::getRandomInt(0, 1);;
            lastGap = (gap - offset) - i;
        } 


        //sometimes we end up with 2 tile left over, so get rid of it
        /*if (m_mapData.tileData[gap + offset + 3] < TileType::Grass)
        {
            m_mapData.tileData[gap + offset + 2] = TileType::LightSand + getRandInt(0, 1);
        }*/
    }
}

void IslandGenerator::processFoliage()
{
    //calc the foliage positions
    m_mapData.foliageCount = 0;
    bool inGrass = false;
    for (auto i = 0u; i < Global::TileCount; ++i)
    {
        if (m_mapData.tileData[i] >= TileType::Grass)
        {
            if (!inGrass)
            {
                //start a new data chunk
                auto offset = i % Global::TileCountX;
                auto row = i / Global::TileCountX;

                m_mapData.foliageData[m_mapData.foliageCount] = { 0.f, sf::Vector2f((offset * Global::TileSize) - (Global::TileSize / 2.f), (row * Global::TileSize)) };

                inGrass = true;

            }
            else
            {
                //extend current chunk
                m_mapData.foliageData[m_mapData.foliageCount].width += Global::TileSize;
            }
        }
        else
        {
            if (inGrass)
            {
                //end chunk
                m_mapData.foliageData[m_mapData.foliageCount].width += Global::TileSize;
                inGrass = false;
                m_mapData.foliageCount++;
            }
        }
    }
}

void IslandGenerator::processEdges()
{
    auto tempSet = m_mapData.tileData;
    
    auto offsetVal =
        [&tempSet](std::size_t x, std::size_t y)->int
    {
        return tempSet[y * (Global::TileCountX) + x];
    };

    for (auto y = 1u; y < Global::TileCountY - 1; ++y)
    {
        for (auto x = 1u; x < Global::TileCountX - 1; ++x)
        {
            std::size_t i = y * Global::TileCountX + x;

            auto TL = offsetVal(x, y);
            auto TR = offsetVal(x + 1, y);
            auto BL = offsetVal(x, y + 1);
            auto BR = offsetVal(x + 1, y + 1);

            auto hTL = TL >> 1;
            auto hTR = TR >> 1;
            auto hBL = BL >> 1;
            auto hBR = BR >> 1;

            auto saddle = ((TL & 1) + (TR & 1) + (BL & 1) + (BR & 1) + 1) >> 2;
            auto shape = (hTL & 1) | (hTR & 1) << 1 | (hBL & 1) << 2 | (hBR & 1) << 3;
            auto ring = (hTL + hTR + hBL + hBR) >> 2;

            auto row = (ring << 1) | saddle;
            auto col = shape - (ring & 1);
            auto index = row * 15 + col;

            m_mapData.tileData[i] = index;
        }
    }
    m_edgesProcessed = true;
}