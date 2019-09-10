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

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Transformable.hpp>

#include <array>

struct Particle final : public sf::Transformable
{
    static constexpr float TotalLife = 1.8f;
    sf::Color colour = sf::Color::White;
    float lifeTime = TotalLife;
    float rotation = 0.f;
    sf::Vector2f velocity;
    //sf::Vector2f defaultVelocity; //temp remove this
};

struct Explosion final
{
    std::array<Particle, 12u> fire;
    std::array<Particle, 6u> smoke;

    enum
    {
        Dirt,
        Fire
    }type = Fire;
};

class ExplosionSystem : public xy::System
{
public:
    explicit ExplosionSystem(xy::MessageBus&);

    void process(float) override;

private:
    void onEntityAdded(xy::Entity) override;

};
