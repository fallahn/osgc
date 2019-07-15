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

struct Enemy final
{
    enum
    {
        Normal, Dying, Dead
    }state = Normal;

    enum
    {
        Crawler, Bird, Walker, Egg, Orb
    }type = Crawler;

    sf::Vector2f velocity;
    sf::Vector2f start;
    sf::Vector2f end;

    float stateTime = 0.f;
    float targetTime = 0.f;
    static constexpr float DyingTime = 3.f;

    static constexpr float CrawlerVelocity = 100.f;
    static constexpr float BirdVelocity = 200.f;
    static constexpr float WalkerVelocity = 110.f;
    static constexpr float DeathImpulse = 700.f;
    static constexpr float EggLife = 4.f;

    void kill()
    {
        stateTime = 0.f;
        velocity.y = -DeathImpulse;
        state = Dying;
    }
};

class EnemySystem final : public xy::System 
{
public:
    explicit EnemySystem(xy::MessageBus&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

private:
    void processCrawler(xy::Entity, float);
    void processBird(xy::Entity, float);
    void processWalker(xy::Entity, float);
    void processEgg(xy::Entity, float);
    void processOrb(xy::Entity, float);
    void processDying(xy::Entity, float);

    void onEntityAdded(xy::Entity) override;
};