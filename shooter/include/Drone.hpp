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
#include <xyginext/ecs/components/Camera.hpp>

#include <SFML/System/Vector2.hpp>

#include <cstdint>

struct Drone final
{
    static constexpr std::int32_t StartLives = 5;
    static constexpr std::int32_t StartAmmo = 25;
    static constexpr float StartHealth = 100.f;
    static constexpr float StartBattery = 100.f;

    sf::Vector2f velocity;
    std::size_t wavetableIndex = 0;
    float health = StartHealth;
    float battery = 100.f;
    std::int32_t ammo = StartAmmo;
    std::int32_t lives = StartLives;
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
        Fire = 0x10,
        Pickup = 0x20
    };
    std::uint16_t inputFlags = 0;

    void reset()
    {
        wavetableIndex = 0;
        state = Drone::State::Flying;
        height = ConstVal::DroneHeight;
        gravity = 0.f;
        //battery = 100.f; //do this manually
        camera.getComponent<xy::Camera>().setZoom(1.f);
    }
};

class DroneSystem final : public xy::System
{
public:
    explicit DroneSystem(xy::MessageBus&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

private:

    std::vector<float> m_wavetable;

    void processFlying(xy::Entity, float);
    void processPickingUp(xy::Entity, float);
    void processDying(xy::Entity, float);

    void spawnBomb(sf::Vector2f position, sf::Vector2f veclocity);

    void updateAmmoBar(Drone);
    void updateBatteryBar(Drone);
};