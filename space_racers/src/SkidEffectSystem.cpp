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

#include "SkidEffectSystem.hpp"
#include "VehicleSystem.hpp"
#include "InputBinding.hpp"
#include "Util.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/ParticleEmitter.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    const float MinVelocity = 940.f;
    const float MinVelocitySqr = MinVelocity * MinVelocity;

    const sf::Time releaseTime = sf::seconds(0.01f);
    const sf::Time soundTime = sf::seconds(0.6f);
    const sf::Vector2f skidSize(14.f, 6.f);

    const std::array<sf::Vector2f, 4u> quad =
    {
        -skidSize,
        sf::Vector2f(skidSize.x, -skidSize.y),
        skidSize,
        sf::Vector2f(-skidSize.x, skidSize.y)
    };

    const float wheelSpacing = 12.f;
}

SkidEffectSystem::SkidEffectSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(SkidEffectSystem))
{
    requireComponent<SkidEffect>();
    requireComponent<xy::Drawable>();
}

//public
void SkidEffectSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& skid = entity.getComponent<SkidEffect>();
        for (auto& mark : skid.skidmarks)
        {
            mark.lifetime -= dt;
        }

        const auto& vehicle = skid.parent.getComponent<Vehicle>();
        if (vehicle.stateFlags == (1 << Vehicle::Normal))
        {
            auto len2 = xy::Util::Vector::lengthSquared(vehicle.velocity);

            if (((vehicle.history[vehicle.lastUpdatedInput].flags & (InputFlag::Left | InputFlag::Right | InputFlag::Brake)))
                && len2 > MinVelocitySqr)
            {
                skid.parent.getComponent<xy::ParticleEmitter>().start();

                if (skid.releaseTimer.getElapsedTime() > releaseTime)
                {
                    skid.releaseTimer.restart();
                    if (skid.skidmarks[skid.currentSkidmark].lifetime < 0)
                    {
                        skid.skidmarks[skid.currentSkidmark].lifetime = Skidmark::DefaultLifeTime;
                        skid.skidmarks[skid.currentSkidmark].position = skid.parent.getComponent<xy::Transform>().getPosition();
                        skid.skidmarks[skid.currentSkidmark].rotation = skid.parent.getComponent<xy::Transform>().getRotation();
                        skid.currentSkidmark = (skid.currentSkidmark + 1) % SkidEffect::MaxSkids;
                    }
                }
                if (skid.soundTimer.getElapsedTime() > soundTime)
                {
                    skid.soundTimer.restart();
                    auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
                    msg->type = VehicleEvent::Skid;
                    msg->entity = skid.parent;
                }
            }
            else
            {
                skid.parent.getComponent<xy::ParticleEmitter>().stop();
            }
        }
        else
        {
            skid.parent.getComponent<xy::ParticleEmitter>().stop();
        }

        //update the drawable
        //TODO can we index this by skidmark array and save rebuilding the entire thing each frame?
        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();
        verts.clear();

        for (auto& mark : skid.skidmarks)
        {
            if (mark.lifetime > 0)
            {
                float alpha = mark.lifetime / Skidmark::DefaultLifeTime;
                alpha *= 115.f;

                sf::Color c(15, 15, 15, static_cast<sf::Uint8>(alpha));

                Transform tx;
                tx.setRotation(mark.rotation);
                float offset = 0.f;
                if (skid.wheelCount > 1)
                {
                    offset = -wheelSpacing;
                }
                for (auto i = 0; i < skid.wheelCount; ++i)
                {
                    for (auto& p : quad)
                    {
                        verts.emplace_back(tx.transformPoint(p.x, p.y + offset) + mark.position, c);
                    }
                    offset += wheelSpacing * 2.f;
                }
            }
        }

        if (!verts.empty())
        {
            drawable.updateLocalBounds();
        }
    }
}

//private
void SkidEffectSystem::onEntityAdded(xy::Entity entity)
{
    entity.getComponent<SkidEffect>().skidmarks.resize(SkidEffect::MaxSkids);
}