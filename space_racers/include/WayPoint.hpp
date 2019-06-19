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

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include <cstdlib>

struct WayPoint final
{
    std::int32_t id = 0;
    sf::Vector2f nextPoint; //points to next waypoint
    float nextDistance = 0.f; //length of above
    //which way should the vehicle point if it spawns here?
    float rotation = 0.f;
    //how far around the track we are if this is our node
    float trackDistance = 0.f;
};
