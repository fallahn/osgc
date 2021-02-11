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

#include "ShakerSystem.hpp"
#include "MessageIDs.hpp"
#include "Collision.hpp"

#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/util/Wavetable.hpp>
#include <xyginext/util/Random.hpp>

namespace
{
    std::vector<float> wavetable;
    const float MaxMagnitude = 3.f;
}

ShakerSystem::ShakerSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ShakerSystem))
{
    requireComponent<xy::Transform>();
    requireComponent<Shaker>();

    wavetable = xy::Util::Wavetable::sine(20.f);
}

//public
void ShakerSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::StarMessage)
    {
        const auto& data = msg.getData<StarEvent>();
        if (data.type == StarEvent::HitItem
            && (data.collisionShape & CollisionShape::Text))
        {
            auto entity = data.entityHit;
            entity.getComponent<Shaker>().magnitude = MaxMagnitude;
        }
    }
}

void ShakerSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& shaker = entity.getComponent<Shaker>();

        if (shaker.magnitude > 0)
        {
            sf::Vector2f offset = shaker.basePosition;
            offset.x += wavetable[shaker.index] * shaker.magnitude;
            shaker.index = (shaker.index + 1) % wavetable.size();
            shaker.magnitude -= dt * MaxMagnitude * 2.f;

            entity.getComponent<xy::Transform>().setPosition(offset);

            if (shaker.magnitude < 0)
            {
                shaker.magnitude = 0.f;
            }
        }
    }
}

//private
void ShakerSystem::onEntityAdded(xy::Entity entity)
{
    auto& shaker = entity.getComponent<Shaker>();
    shaker.basePosition = entity.getComponent<xy::Transform>().getPosition();
    shaker.index = xy::Util::Random::value(0u, wavetable.size() - 1);
}
