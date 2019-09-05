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

#include "ClientFlareSystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Wavetable.hpp>

#include <SFML/Graphics/Vertex.hpp>

#include <array>

namespace
{
    const std::array<sf::Vertex, 4u> flareVerts = 
    {
        sf::Vertex({-8.f, 16.f}, {8.f, 0.f}),
        sf::Vertex({8.f, 16.f}, {24.f, 0.f}),
        sf::Vertex({8.f, -16.f}, {24.f, 32.f}),
        sf::Vertex({-8.f, -16.f}, {8.f, 32.f})
    };

    const std::array<sf::Vertex, 4u> particleVerts = 
    {
        sf::Vertex({-2.f, 2.f}, {112.f, 16.f}),
        sf::Vertex({2.f, 2.f}, {115.f, 16.f}),
        sf::Vertex({2.f, -2.f}, {115.f, 19.f}),
        sf::Vertex({-2.f, -2.f}, {112.f, 19.f})
    };

    const float Velocity = 300.f;
    const float Acceleration = 800.f;
}

ClientFlareSystem::ClientFlareSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ClientFlareSystem)),
    m_tableIndex(0)
{
    requireComponent<ClientFlare>();
    requireComponent<xy::Drawable>();

    m_wavetable = xy::Util::Wavetable::sine(10.f, 5.f);
}

void ClientFlareSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& flare = entity.getComponent<ClientFlare>();
        flare.launchTime = std::min(1.f, flare.launchTime + (dt * 2.f));
        flare.position.x += (1.f - flare.launchTime);
        flare.position.y = std::min(flare.position.y + (Velocity + (Acceleration * flare.launchTime)) * dt, 400.f);

        if (flare.emitDuration > 0)
        {
            //spawn a particle if time
            flare.nextParticleTime -= dt;
            if (flare.nextParticleTime < 0)
            {
                flare.nextParticleTime = ClientFlare::NextParticle + (m_wavetable[m_tableIndex] * 0.001f);

                flare.particles.push_back(ClientFlare::Particle());
                flare.particles.back().position = flare.position;
                flare.particles.back().position.x += m_wavetable[m_tableIndex];
            }
            flare.emitDuration -= dt;
        }

        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();
        verts.clear();

        //update the actual flare
        for (auto v : flareVerts)
        {
            verts.push_back(v);
            verts.back().position += flare.position;
        }

        //update particles
        for (auto& p : flare.particles)
        {
            float ratio = xy::Util::Math::clamp(p.lifeTime / ClientFlare::Particle::MaxLife, 0.f, 1.f);

            p.position.x += 3.f * dt * ratio;
            p.lifeTime = std::max(0.f, p.lifeTime - dt);

            //calc alpha based on lifetime
            sf::Uint8 alpha = static_cast<sf::Uint8>(220.f * ratio);
            sf::Color c(255, 255, 255, alpha);
            float scale = (0.5f + (0.5f - (ratio * 0.5f)));

            for (auto v : particleVerts)
            {
                verts.push_back(v);
                verts.back().position *= scale; //grow with age
                verts.back().position += p.position;
                verts.back().color = c;
            }
        }

        drawable.updateLocalBounds();
    }

    m_tableIndex = (m_tableIndex + 1) % m_wavetable.size();
}