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

#include <xyginext/ecs/System.hpp>

#include <SFML/System/Vector2.hpp>

struct Seagull final
{
    sf::Vector2f velocity;
    bool moves = true;
    bool enabled = true; //disable at night

    float currentTime = 0.f;
    float callTime = 0.f;
};

class SeagullSystem final : public xy::System
{
public:
    explicit SeagullSystem(xy::MessageBus&);

    void process(float) override;

private:

};