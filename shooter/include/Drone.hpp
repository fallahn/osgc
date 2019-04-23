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

#include "GameConsts.hpp"

#include <xyginext/ecs/System.hpp>

#include <SFML/System/Vector2.hpp>

#include <cstdint>

struct Drone final
{
    sf::Vector2f velocity;
    std::int32_t health = 100;
    float battery = 100.f;
    std::int32_t ammo = 40;
    std::int32_t lives = 5;
    float height = ConstVal::DroneHeight;
    float gravity = 0.f;
    xy::Entity camera;

    enum class State
    {
        Flying, PickingUp, Dying, Dead
    }state = State::Flying;

    enum InputFlags
    {
        Up = 0x1,
        Down = 0x2,
        Left = 0x4,
        Right = 0x8,
        Fire = 0x10
    };
    std::uint16_t inputFlags = 0;
};

class DroneSystem final : public xy::System
{
public:
    explicit DroneSystem(xy::MessageBus&);

    void process(float) override;

private:

    void processFlying(xy::Entity, float);
    void processPickingUp(xy::Entity, float);
    void processDying(xy::Entity, float);

    void spawnBomb(sf::Vector2f position, sf::Vector2f veclocity);
};