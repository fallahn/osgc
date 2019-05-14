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

#include <cstdint>

namespace GameConst
{
    static const std::int32_t TrackRenderDepth = -10;
    static const std::int32_t VehicleRenderDepth = 0;
    static const std::int32_t RoidRenderDepth = 5;

    static const sf::FloatRect CarSize(0.f, 0.f, 135.f, 77.f);
    static const sf::FloatRect BikeSize(0.f, 0.f, 132.f, 40.f);
    static const sf::FloatRect ShipSize(0.f, 0.f, 132.f, 120.f);
}