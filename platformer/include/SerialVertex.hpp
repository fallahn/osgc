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

//used to serialise/deserialise sf::Vertex data to binary files

#include <cstdint>

//note the UVs are stored as normalised OpenGL
//values, not SFML's absolute values
struct SerialVertex final
{
    float posX = 0.f;
    float posY = 0.f;
    std::uint8_t normX = 0; //< the normal is packed in the same was as a normal map texture. Because fudgery.
    std::uint8_t normY = 0;
    std::uint8_t normZ = 0;
    std::uint8_t heightMultiplier = 0; //< each model has a 'height' value representing a maximum Z value. This is a normalised multiplier of that
    float texU = 0.f;
    float texV = 0.f;
};
