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

#include <xyginext/ecs/Scene.hpp>

struct Crab final
{
    sf::Vector2f spawnPosition;
    float maxTravel = 15.f; //in each direction
    enum class State
    {
        Running, Digging
    }state = State::Running;
    float thinkTime = 1.f;
};

class CrabSystem final : public xy::System 
{
public:
    explicit CrabSystem(xy::MessageBus& mb);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:

};