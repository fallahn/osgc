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

#include "CollisionObject.hpp"

#include <xyginext/ecs/System.hpp>

#include <array>

struct Input final
{
    std::int32_t timestamp = 0;
    float steeringMultiplier = 1.f;
    float accelerationMultiplier = 1.f;
    std::int16_t flags = 0;
};

using History = std::array<Input, 120>;

struct Vehicle final
{
    History history;
    std::size_t currentInput = 0;
    std::size_t lastUpdatedInput = 0;

    sf::Vector2f velocity;
    float anglularVelocity = 0.f; //rads

    //allows swapping out different behaviour
    //for different vehicle types
    struct Settings final
    {
        Settings() {};
        Settings(float ad, float ts, float d, float a)
            : angularDrag(ad), turnSpeed(ts), drag(d), acceleration(a) {}

        float angularDrag = 0.8f;
        float turnSpeed = 0.717f; //rads per second

        float drag = 0.877f; //multiplier
        float acceleration = 84.f; //units per second
        float maxSpeed() { return drag * (acceleration / (1.f - drag)); }
        float maxSpeedSqr() { auto speed = maxSpeed(); return speed * speed; }

        static constexpr float brakeStrength = 0.92f; //multiplier
    }settings;

    enum Type
    {
        Car, Bike, Ship
    }type = Car;

    enum State
    {
        Normal = 0,
        Falling,
        Explosing
    };

    std::uint16_t collisionFlags = 0;
    std::uint16_t stateFlags = 1;

    std::int32_t waypointID = -1; //because the first we hit is 0
    std::int32_t waypointCount = 0;
    sf::Vector2f waypointPosition;
};

struct ClientUpdate;
class VehicleSystem final : public xy::System 
{
public: 
    explicit VehicleSystem(xy::MessageBus& mb);

    void process(float) override;

    void reconcile(const ClientUpdate&, xy::Entity);

private:

    void processInput(xy::Entity);
    void applyInput(xy::Entity, float);

    float getDelta(const History&, std::size_t);

    void doCollision(xy::Entity);
    void resolveCollision(xy::Entity, xy::Entity, Manifold);

    void onEntityAdded(xy::Entity) override;
};