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

struct Crate final
{
    sf::Vector2f spawnPosition;
    sf::Vector2f velocity;

    float stateTime = 0.f;

    enum State
    {
        Idle, Falling, Carried, Dead
    }state = Falling;

    xy::Entity platform; //parent if crate is resting on a moving platform

    static constexpr float Drag = 0.8f;
    static constexpr float DeadTime = 2.f;
};

struct SharedData;
class CrateSystem final : public xy::System
{
public:
    CrateSystem(xy::MessageBus&, SharedData&);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:

    SharedData& m_sharedData;

    void updateFalling(xy::Entity, float);
    void updateIdle(xy::Entity, float);
    void updateDead(xy::Entity, float);

    void detachPlatform(xy::Entity);

    void applyVelocity(xy::Entity, float);
    void doCollision(xy::Entity);
    void resolveCollision(xy::Entity, xy::Entity, sf::FloatRect);
    void kill(xy::Entity);
};