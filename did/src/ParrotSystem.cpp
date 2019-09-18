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

#include "ParrotSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float Speed = 180.f;
    sf::Vector2f HomePosition = { 0.f, -20.f };
}

ParrotSystem::ParrotSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ParrotSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<ParrotFlock>();
}

void ParrotSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& flock = entity.getComponent<ParrotFlock>();
        auto& vertices = entity.getComponent<xy::Drawable>().getVertices();
        vertices.clear();

        //triggered server side by proximity 
        if (flock.state == ParrotFlock::Roosting)
        {
            for (auto& parrot : flock.parrots)
            {
                parrot.setPosition(HomePosition);
            }
        }
        else
        {
            for (auto& parrot : flock.parrots)
            {
                //update position
                parrot.move(parrot.velocity * dt);

                //check if out of view then count until all parrots leave and we reset
                if (parrot.getPosition().y > 320)
                {
                    //parrot.setPosition(HomePosition);
                    flock.roostCount++;

                    if (flock.roostCount == ParrotFlock::ParrotCount)
                    {
                        flock.state = ParrotFlock::Roosting;
                    }
                }

                //update texture
                parrot.currentTime += dt;
                if (parrot.currentTime > Parrot::TimePerFrame)
                {
                    parrot.currentTime = 0.f;
                    parrot.currentFrame = (parrot.currentFrame + 1) % Parrot::MaxFrames;
                    parrot.textureRect.left = 32.f * parrot.currentFrame;

                    parrot.vertices[0].texCoords = { parrot.textureRect.left, parrot.textureRect.top + parrot.textureRect.height };
                    parrot.vertices[1].texCoords = { parrot.textureRect.left + parrot.textureRect.width, parrot.textureRect.top + parrot.textureRect.height };
                    parrot.vertices[2].texCoords = { parrot.textureRect.left + parrot.textureRect.width, parrot.textureRect.top };
                    parrot.vertices[3].texCoords = { parrot.textureRect.left, parrot.textureRect.top };
                }

                //update vertex array
                for (auto v : parrot.vertices)
                {
                    v.position = parrot.getTransform().transformPoint(v.position);
                    vertices.push_back(v);
                }
            }
            entity.getComponent<xy::Drawable>().updateLocalBounds();
        }        
    }
}

void ParrotSystem::onEntityAdded(xy::Entity entity)
{
    auto& flock = entity.getComponent<ParrotFlock>();

    auto left = xy::Util::Random::value(0, 1);

    std::size_t i = 0;
    for (auto& parrot : flock.parrots)
    {
        if (left)
        {
            parrot.velocity = { xy::Util::Random::value(-1.f, -0.2f), xy::Util::Random::value(0.2f, 1.f) };
        }
        else
        {
            parrot.velocity = { xy::Util::Random::value(0.2f, 1.f), xy::Util::Random::value(0.2f, 1.f) };
            parrot.scale(-1.f, 1.f);
        }
        parrot.velocity = xy::Util::Vector::normalise(parrot.velocity) * Speed;

        float scale = xy::Util::Random::value(0.6f, 1.f);
        parrot.scale(scale, scale);

        parrot.vertices[0] = sf::Vertex(sf::Vector2f(-16.f, -16.f), sf::Vector2f());
        parrot.vertices[1] = sf::Vertex(sf::Vector2f(16.f, -16.f), sf::Vector2f(32.f, 0.f));
        parrot.vertices[2] = sf::Vertex(sf::Vector2f(16.f, 16.f), sf::Vector2f(32.f, 32.f));
        parrot.vertices[3] = sf::Vertex(sf::Vector2f(-16.f, 16.f), sf::Vector2f(0.f, 32.f));

        i++;
        parrot.setPosition(HomePosition + sf::Vector2f(i * 12.f, i * -14.f));
    }
}