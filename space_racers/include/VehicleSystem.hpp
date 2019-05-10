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
#ifdef XY_DEBUG
#include <xyginext/gui/GuiClient.hpp>
#endif //XY_DEBUG

#include <array>

struct Input final
{
    sf::Int64 timestamp = 0;
    sf::Uint16 flags = 0;
};

using History = std::array<Input, 120>;

struct Vehicle final
{
    History history;
    std::size_t currentInput = 0;
    std::size_t lastUpdatedInput = 0;

    sf::Vector2f velocity;
    float anglularVelocity = 0.f; //rads

    //technically these are const vals
    //but may be un-consted to create
    //vehicular variations
    static constexpr float angularDrag = 0.9f;
    static constexpr float turnSpeed = 0.76f; //rads per second

    static constexpr float drag = 0.93f; //multiplier
    static constexpr float acceleration = 84.f; //units per second
    static constexpr float maxSpeed() { return drag * (acceleration / (1.f - drag)); }
    static constexpr float maxSpeedSqr() { return maxSpeed() * maxSpeed(); }

    static constexpr float brakeStrength = 0.15f; //multiplier
};

class VehicleSystem final : public xy::System 
#ifdef XY_DEBUG
    , public xy::GuiClient
#endif //XY_DEBUG
{
public: 
    explicit VehicleSystem(xy::MessageBus& mb);

    void process(float) override;

private:

    void processInput(xy::Entity);
    void applyInput(xy::Entity, float);
};