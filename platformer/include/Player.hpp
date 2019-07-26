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

#include "AnimationMap.hpp"

#include <xyginext/ecs/System.hpp>

struct Player final
{
    std::uint16_t input = 0;
    std::uint16_t prevInput = 0;
    float accelerationMultiplier = 0.f;

    enum
    {
        Running, Falling,
        Dying, Dead
    }state = Falling;
    
    sf::Vector2f velocity;
    static constexpr float Acceleration = 120.f;
    static constexpr float Drag = 0.81f;

    AnimationMap<AnimID::Player::Count> animations = {};

    std::int32_t lastCheckpoint = 0;

    xy::Entity carriedItem;
    xy::Entity platform; //moving platform if on one

    float stateTime = 0.f;
    static constexpr float DyingTime = 1.f;
    static constexpr float DeadTime = 1.f;
    static constexpr float InvincibleTime = 0.5f;

    bool hasShield = false;
};

struct Dialogue final
{
    bool expired = false;
    std::string file;
};

struct SharedData;
class PlayerSystem final : public xy::System
{
public:
    PlayerSystem(xy::MessageBus&, SharedData&);

    void process(float) override;

    void setBounds(sf::FloatRect bounds) { m_bounds = bounds; }

private:

    SharedData& m_sharedData;
    sf::FloatRect m_bounds;

    void processFalling(xy::Entity, float);
    void processRunning(xy::Entity, float);
    void processDying(xy::Entity, float);
    void processDead(xy::Entity, float);

    void doCollision(xy::Entity, float);
    void resolveCollision(xy::Entity, xy::Entity, sf::FloatRect);
    void applyVelocity(xy::Entity, float);

    void kill(xy::Entity);
};