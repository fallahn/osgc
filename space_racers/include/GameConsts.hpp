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
}