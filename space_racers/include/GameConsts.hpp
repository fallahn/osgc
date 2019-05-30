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

#include <SFML/Graphics/Color.hpp>

#include <cstdint>
#include <vector>

namespace GameConst
{
    static const float InvincibleTime = 1.f;

    static const std::int32_t BackgroundRenderDepth = -20;
    static const std::int32_t TrackRenderDepth = -10;
    static const std::int32_t VehicleRenderDepth = 0;
    static const std::int32_t RoidRenderDepth = 5;

    static const sf::FloatRect CarSize(0.f, 0.f, 135.f, 77.f);
    static const sf::FloatRect BikeSize(0.f, 0.f, 132.f, 40.f);
    static const sf::FloatRect ShipSize(0.f, 0.f, 132.f, 120.f);

    //how near to the front of the vehicle the pivot point is
    //where 0 is at the back, 0.5 in the dead centre and 1 at the front
    static const float VehicleCentreOffset = 0.45f;

    static const std::vector<sf::Vector2f> CarPoints =
    {
        sf::Vector2f(),
        sf::Vector2f(CarSize.width, 0.f),
        sf::Vector2f(CarSize.width, CarSize.height),
        sf::Vector2f(0.f, CarSize.height)
    };

    static const std::vector<sf::Vector2f> BikePoints =
    {
        sf::Vector2f(),
        sf::Vector2f(BikeSize.width, 0.f),
        sf::Vector2f(BikeSize.width, BikeSize.height),
        sf::Vector2f(0.f, BikeSize.height)
    };

    static const std::vector<sf::Vector2f> ShipPoints =
    {
        sf::Vector2f(),
        sf::Vector2f(ShipSize.width, 0.f),
        sf::Vector2f(ShipSize.width, ShipSize.height),
        sf::Vector2f(0.f, ShipSize.height)
    };

    static const float SpawnSpacing = ShipSize.height * 1.2f;
    static const std::array<sf::Vector2f, 4u> SpawnPositions =
    {
        sf::Vector2f(0.f, SpawnSpacing * 0.5f),
        sf::Vector2f(0.f, -SpawnSpacing * 0.5f),
        sf::Vector2f(0.f, -SpawnSpacing * 1.f),
        sf::Vector2f(0.f, SpawnSpacing * 1.5f)
    };

    enum Colour
    {
        Cyan,
        Magenta,
        Green,
        Purple,
        None,
        Count
    };
    static const std::array<sf::Color, Colour::Count> colours =
    {
        sf::Color(0,255,255),
        sf::Color(225,0,255),
        sf::Color(0,255,0),
        sf::Color(255,128,255),
        sf::Color::White
    };

    namespace PlayerColour
    {
        static const std::array<sf::Color, 4u> Light =
        {
            sf::Color(255u, 0u, 10u),
            sf::Color(0u, 127u, 255u),
            sf::Color(63u, 0u, 255u),
            sf::Color(0u, 191u, 127u)

        };

        static const std::array<sf::Color, 4u> Dark =
        {
            sf::Color(76u, 0u, 0u),
            sf::Color(0u, 37u, 76u),
            sf::Color(18u, 0u, 76u),
            sf::Color(0u, 54u, 37u)
        };
    }

    enum FilterFlags
    {
        Neon = 0x1,
        Normal = 0x2,


        All = Neon | Normal
    };

    namespace TrackLayer
    {
        enum
        {
            Track,
            Neon,
            Normal
        };
    }
    static const sf::Vector2u LargeBufferSize(1920, 1080);
    static const sf::Vector2u MediumBufferSize(960, 540);
    static const sf::Vector2u SmallBufferSize(480u, 270u);
}