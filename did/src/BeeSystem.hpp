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

#include "ServerStormDirector.hpp"

#include <xyginext/ecs/System.hpp>

#include <SFML/System/Vector2.hpp>

struct Bee final
{
    float nextScanTime = 0.f;
    float nextStingTime = 0.f;
    float wakeTime = 0.f;
    static constexpr float ScanTime = 2.f; //how frequently to scan for trespassers
    static constexpr float StingTime = 1.f;
    static constexpr float WakeDuration = 0.7f;
    sf::Vector2f homePosition;
    xy::Entity target;
    
    enum class State
    {
        Sleeping, Waking, Awake
    }state = State::Awake;
};

class BeeSystem : public xy::System
{
public:
    explicit BeeSystem(xy::MessageBus&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

    void onEntityAdded(xy::Entity) override;

private:

    float m_dayPosition;
    StormDirector::State m_weather = StormDirector::Dry;

    void move(xy::Entity, sf::Vector2f target, float);
    bool isDayTime() const;
};
