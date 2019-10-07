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

#ifdef XY_DEBUG
#define DD_DEBUG
#endif

#include <xyginext/Config.hpp>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstdint>
#include <cstddef>
#include <array>

namespace Global
{
    //map properties
    static const sf::Vector2f IslandSize(1536.f, 1536.f);
    static const float TileSize = 32.f;
    static const float PlayerCameraOffset = 152.f;

    static const std::size_t TileCountX = 48;
    static const std::size_t TileCountY = TileCountX;
    static const std::size_t TileCount = TileCountX * TileCountY;

    static const std::size_t MaxFoliage = 24; //assumes absolute max 3 blocks per row
    static const float DayNightSeconds = 600.f;
    static const float DayCycleOffset = 0.8f; //initial time of day is offset by this

    //3D sprites have their depth updated to their Y position
    //unless they are below this depth
    static const std::int32_t MaxSortingDepth = -5000;

    //network settings
    static const std::uint16_t GamePort = 27015;
    //static const std::uint16_t AuthPort = 8766;
    //static const std::uint16_t MasterServerPort = 27016;

    //ui consts
    static const std::uint32_t SmallTextSize = 30;
    static const std::uint32_t MediumTextSize = 80;
    static const std::uint32_t LargeTextSize = 160;
    static const std::uint32_t LobbyTextSize = 50;
    
    static const sf::Color InnerTextColour(236, 178, 19);
    static const sf::Color OuterTextColour(84,63,42);// (65, 33, 31);
    static const sf::Color ButtonTextColour(113, 89, 63);

    //collision properties
    static const sf::FloatRect PlayerBounds(-7.f, -7.f, 14.f, 14.f); //the sprite scale ends up doubling this
    static const sf::FloatRect LanternBounds(-16.f, -16.f, 32.f, 32.f);
    static const sf::FloatRect BoatBounds(-(TileSize * 1.1f), -(TileSize / 2.5f), TileSize * 2.2f, TileSize * 0.7f);
    static const sf::FloatRect CollectibleBounds((-Global::TileSize / 3.f), (-Global::TileSize / 3.f), (Global::TileSize / 3.f) * 2.f, (Global::TileSize / 3.f) * 2.f);
    static const sf::FloatRect SkullShieldBounds(-Global::TileSize, -(Global::TileSize / 2.f), Global::TileSize * 2.f, Global::TileSize);

    static const float CarryOffset = 13.f;// 26.f;

    static const float LightRadius = 64.f;
    //boat positions relative to edge of map
    static const float BoatXOffset = TileSize * 6.f;
    static const float BoatYOffset = TileSize * 6.f;
    static const float PlayerSpawnOffset = TileSize;

    static const std::array<sf::Vector2f, 4u> BoatPositions =
    {
        sf::Vector2f(Global::BoatXOffset, Global::BoatYOffset),
        sf::Vector2f(Global::IslandSize.x - Global::BoatXOffset, Global::BoatYOffset),
        sf::Vector2f(Global::IslandSize.x - Global::BoatXOffset, Global::IslandSize.y - Global::BoatYOffset - Global::PlayerSpawnOffset),
        sf::Vector2f(Global::BoatXOffset, Global::IslandSize.y - Global::BoatYOffset - Global::PlayerSpawnOffset)
    };

    static const std::array<sf::Vector2f, 4u> SpawnOffsets =
    {
        sf::Vector2f(Global::BoatXOffset / 4.f, Global::PlayerSpawnOffset),
        sf::Vector2f(-Global::BoatXOffset / 4.f, Global::PlayerSpawnOffset),
        sf::Vector2f(-Global::BoatXOffset / 4.f, -Global::PlayerSpawnOffset),
        sf::Vector2f(Global::BoatXOffset / 4.f, -Global::PlayerSpawnOffset)
    };

    static const float BoatRadius = 36.f;

    static const int TileExclusionX = 8;
    static const int TileExclusionY = 5;
    const std::array<sf::Vector2i, 4u> TileExclusionCorners =
    {
        sf::Vector2i(), sf::Vector2i(-TileExclusionX, 0),
        sf::Vector2i(-TileExclusionX, -TileExclusionY), sf::Vector2i(0, -TileExclusionY)
    };

    //default character names
    static const std::array<sf::String, 4u> PlayerNames =
    {
        sf::String("Sir Rodney Boardshine"),
        sf::String("Coiffured Jean"),
        sf::String("Helena Squinteye"),
        sf::String("Barnacle Lars")
    };
    static const std::size_t MaxNameSize = 40 * sizeof(sf::Uint32); //because utf32

    static const std::array<sf::Color, 4u> PlayerColours = 
    {
        sf::Color(128,25,8),
        sf::Color(51,128,8),
        sf::Color(85,8,128),
        sf::Color(8,111,128)
    };

    //UI stuffs
    static const sf::Vector2f DefaultHealthPosition(0.f, xy::DefaultSceneSize.y - 84.f);
    static const sf::Vector2f HiddenHealthPosition(0.f, xy::DefaultSceneSize.y + 10.f);
    static const std::string TitleFont("assets/fonts/Avast-Regular.ttf");
    static const std::string BoldFont("assets/fonts/BlanketBlk.otf");
    static const std::string FineFont("assets/fonts/Blanket.otf");

    static const sf::Vector2f MapOffPosition(20.f, xy::DefaultSceneSize.y * 0.78f);
    static const sf::Vector2f MapOnPosition = MapOffPosition;// (20.f, xy::DefaultSceneSize.y / 2.f);
    static const sf::Vector2f MapZoomPosition((xy::DefaultSceneSize.x / 2.f) - 512.f, xy::DefaultSceneSize.y / 2.f);

    //point values
    static const std::uint16_t TreasureValue = 40;

    //network stuff - TODO this should probably be an enum
    static const std::size_t NetworkChannels = 4;
    static const std::uint8_t AnimationChannel = 1;
    static const std::uint8_t ReliableChannel = 2;
    static const std::uint8_t LowPriorityChannel = 3;
}