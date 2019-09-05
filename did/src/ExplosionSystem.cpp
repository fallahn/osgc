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

#include "ExplosionSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float RotationSpeed = 400.f;
    const float Friction = 0.6f;
    const sf::Vector2f Gravity(0.f, -520.f);
    const std::array<float, 3> DefaultFireColour = { 255.f, 120.f, 10.f };
    const std::array<float, 3> DefaultSmokeColour = { 255.f, 255.f, 255.f };

    const float FireOffset = 4.f;
    const float SmokeOffset = 18.f;

    const float QuadSize = 3.f;
    const std::array<sf::Vector2f, 4> QuadPoints = 
    {
        sf::Vector2f(-QuadSize, -QuadSize),
        sf::Vector2f(QuadSize, -QuadSize),
        sf::Vector2f(QuadSize, QuadSize),
        sf::Vector2f(-QuadSize, QuadSize)
    };

    const std::array<sf::Vector2f, 8> QuadUVs =
    {
        sf::Vector2f(),
        sf::Vector2f(16.f, 0.f),
        sf::Vector2f(16.f, 16.f),
        sf::Vector2f(0.f, 16.f),

        sf::Vector2f(16.f, 0.f),
        sf::Vector2f(32.f, 0.f),
        sf::Vector2f(32.f, 16.f),
        sf::Vector2f(16.f, 16.f)
    };
}

ExplosionSystem::ExplosionSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ExplosionSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<Explosion>();
}

//public
void ExplosionSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& explosion = entity.getComponent<Explosion>();
        auto& verts = entity.getComponent<xy::Drawable>().getVertices();

        auto i = 0u;
        for (auto& particle : explosion.fire)
        {
            particle.move(particle.velocity * dt);
            particle.velocity += Gravity * dt;

            particle.lifeTime = std::max(0.f, particle.lifeTime - dt);

            float ratio = particle.lifeTime / Particle::TotalLife;
            /*particle.colour.r = static_cast<sf::Uint8>(DefaultFireColour[0] * ratio);
            particle.colour.g = static_cast<sf::Uint8>(DefaultFireColour[1] * ratio);
            particle.colour.b = static_cast<sf::Uint8>(DefaultFireColour[2] * ratio);*/
            particle.colour.a = static_cast<sf::Uint8>(255.f * ratio);
 
            auto tx = particle.getTransform();
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[0]), particle.colour, QuadUVs[0]);
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[1]), particle.colour, QuadUVs[1]);
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[2]), particle.colour, QuadUVs[2]);
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[3]), particle.colour, QuadUVs[3]);

            //bounce
            auto position = particle.getPosition();
            if (position.y < 0)
            {
                particle.velocity.y = -particle.velocity.y;
                particle.velocity *= Friction;
                particle.setPosition(position.x, 0.f);

                particle.rotation *= Friction;
            }

            //shrink
            particle.scale(0.99f, 0.99f);
            particle.rotate(particle.rotation * dt);

            //temp to reset particles
            /*if (particle.lifeTime == 0)
            {
                particle.lifeTime = Particle::TotalLife;
                particle.setPosition(0.f, FireOffset);
                particle.velocity = particle.defaultVelocity;
                particle.setScale(1.f, 1.f);
                particle.rotation = -particle.velocity.x * RotationSpeed;
            }*/
        }

        //loop through smoke particles and update
        for (auto& particle : explosion.smoke)
        {
            particle.move(particle.velocity * dt);
            particle.velocity *= 0.99f;
            particle.velocity += sf::Vector2f(5.f, 10.f) * dt;

            particle.lifeTime = std::max(0.f, particle.lifeTime - dt);

            float ratio = particle.lifeTime / Particle::TotalLife;
            /*particle.colour.r = static_cast<sf::Uint8>(DefaultSmokeColour[0] * ratio);
            particle.colour.g = static_cast<sf::Uint8>(DefaultSmokeColour[1] * ratio);
            particle.colour.b = static_cast<sf::Uint8>(DefaultSmokeColour[2] * ratio);*/
            particle.colour.a = static_cast<sf::Uint8>(255.f * ratio);

            auto tx = particle.getTransform();
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[0]), particle.colour, QuadUVs[4]);
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[1]), particle.colour, QuadUVs[5]);
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[2]), particle.colour, QuadUVs[6]);
            verts[i++] = sf::Vertex(tx.transformPoint(QuadPoints[3]), particle.colour, QuadUVs[7]);

            //grow
            particle.scale(1.01f, 1.01f);
            particle.rotate(particle.rotation * dt);

            //temp to reset particles
            /*if (particle.lifeTime == 0)
            {
                particle.lifeTime = Particle::TotalLife;
                particle.setPosition(0.f, SmokeOffset);
                particle.velocity = particle.defaultVelocity;
                particle.setScale(1.f, 1.f);
                particle.rotation = -particle.velocity.x;
            }*/
        }

        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }
}

void ExplosionSystem::onEntityAdded(xy::Entity entity)
{
    auto& explosion = entity.getComponent<Explosion>();

    if (explosion.type == Explosion::Fire)
    {
        for (auto& particle : explosion.fire)
        {
            particle.velocity.x = static_cast<float>(xy::Util::Random::value(-20, 20));
            particle.velocity.y = 100.f;
            particle.velocity = xy::Util::Vector::normalise(particle.velocity) * static_cast<float>(xy::Util::Random::value(160, 220));
            particle.rotation = -particle.velocity.x * RotationSpeed;
            //particle.defaultVelocity = particle.velocity; //temp, remove this

            particle.setPosition(0.f, FireOffset); //start at centre of sprite
        }

        //init smoke particles
        for (auto& particle : explosion.smoke)
        {
            particle.velocity.x = static_cast<float>(xy::Util::Random::value(20, 40));
            particle.velocity.y = 100.f;
            particle.velocity = xy::Util::Vector::normalise(particle.velocity) * static_cast<float>(xy::Util::Random::value(30, 40));
            particle.rotation = -particle.velocity.x;

            particle.setPosition(0.f, SmokeOffset); //start at top of sprite
            //particle.defaultVelocity = particle.velocity; //temp, remove this
        }
    }
    else
    {
        for (auto& particle : explosion.fire)
        {
            particle.velocity.x = static_cast<float>(xy::Util::Random::value(-20, 20));
            particle.velocity.y = 100.f;
            particle.velocity = xy::Util::Vector::normalise(particle.velocity) * static_cast<float>(xy::Util::Random::value(60, 120));
            particle.rotation = -particle.velocity.x * RotationSpeed;

            auto colour = static_cast<sf::Uint8>(xy::Util::Random::value(120, 255));
            particle.colour = { colour, colour, colour };
        }

        //init smoke particles
        for (auto& particle : explosion.smoke)
        {
            particle.velocity.x = static_cast<float>(xy::Util::Random::value(20, 40));
            particle.velocity.y = 100.f;
            particle.velocity = xy::Util::Vector::normalise(particle.velocity) * static_cast<float>(xy::Util::Random::value(30, 40));
            particle.rotation = -particle.velocity.x;
        }
    }

    entity.getComponent<xy::Drawable>().getVertices().resize((explosion.fire.size() + explosion.smoke.size()) * 4u);
}