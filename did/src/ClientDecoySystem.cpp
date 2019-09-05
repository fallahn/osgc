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

#include "ClientDecoySystem.hpp"
#include "Sprite3D.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Wavetable.hpp>

#include <SFML/Graphics/Vertex.hpp>

#include <array>

namespace
{
    std::array<sf::Vertex, 4u> vertices = 
    {
        sf::Vertex({-32.f, 64.f}, {0.f, 0.f}),
        sf::Vertex({32.f, 64.f}, {64.f, 0.f}),
        sf::Vertex({32.f, 0.f}, {64.f, 64.f}),
        sf::Vertex({-32.f, 0.f}, {0.f, 64.f})
    };

    const float TargetScale = 0.5f;
    const float InflateSpeed = 4.f;
    const float MaxRotation = 2.f;
}

ClientDecoySystem::ClientDecoySystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ClientDecoySystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<ClientDecoy>();
    requireComponent<Sprite3D>();

    //set up wavetable
    m_waveTable = xy::Util::Wavetable::sine(0.25f);
}

//public
void ClientDecoySystem::process(float dt)
{
    /*
    NOTE as this currently only does animation we're also using it
    to update the animation on skull shields
    */

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& decoy = entity.getComponent<ClientDecoy>();
        if (decoy.state == ClientDecoy::Spawning)
        {
            //inflate animation
            auto& tx = entity.getComponent<xy::Transform>();
            auto scale = tx.getScale();
            auto scaleAmount = (TargetScale - scale.x) * (dt * (InflateSpeed / 2.f));
            scale.x += scaleAmount;
            scale.y -= scaleAmount;
            tx.setScale(scale);

            auto& sprite = entity.getComponent<Sprite3D>();
            auto offset = sprite.verticalOffset * (dt * InflateSpeed);
            sprite.verticalOffset -= offset;

            if (sprite.verticalOffset < 0.5)
            {
                sprite.verticalOffset = 0.f;
                decoy.state = ClientDecoy::Idle;
            }
        }
        else if (decoy.state == ClientDecoy::Idle)
        {
            //gently wobble
            //entity.getComponent<xy::Transform>().setRotation(m_waveTable[decoy.wavetableIndex] * MaxRotation);
            //decoy.wavetableIndex = (decoy.wavetableIndex + 1) % m_waveTable.size();            
        }
        else
        {
            //play burst animation?
        }
    }
}

void ClientDecoySystem::onEntityAdded(xy::Entity entity)
{
    entity.getComponent<xy::Transform>().setScale(0.f, 0.f);
    entity.getComponent<Sprite3D>().verticalOffset = 32.f;
}
