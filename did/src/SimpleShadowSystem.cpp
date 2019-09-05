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

#include "SimpleShadowSystem.hpp"
#include "TorchlightSystem.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

namespace
{
    const float MaxRadius = Global::LightRadius * 1.2f;
    const float MinRadius = Global::LightRadius;

    const float MaxRadiusSqr = MaxRadius * MaxRadius;
    const float MinRadiusSqr = MinRadius * MinRadius;
    const float FadeDistance = MaxRadiusSqr - MinRadiusSqr;
}

SimpleShadowSystem::SimpleShadowSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(SimpleShadowSystem))
{
    requireComponent<xy::Sprite>();
    requireComponent<xy::Transform>();
    requireComponent<SimpleShadow>();
}

//public
void SimpleShadowSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        //destroy if parent is destroyed
        const auto& simpleShadow = entity.getComponent<SimpleShadow>();
        if (!simpleShadow.parent.isValid() || simpleShadow.parent.destroyed())
        {
            getScene()->destroyEntity(entity);
            continue;
        }

        auto position = entity.getComponent<xy::Transform>().getWorldPosition();
        float alpha = 1.f;
        float lastDistance = MaxRadiusSqr;

        //then for each light set trans to whichever is closest
        for (auto light : m_lights)
        {
            auto dist = xy::Util::Vector::lengthSquared(light.getComponent<xy::Transform>().getPosition() - position);

            if (dist < lastDistance)
            {
                lastDistance = dist;
                float amount = std::max(0.f, dist - MinRadiusSqr);
                alpha = xy::Util::Math::clamp(amount / FadeDistance, 0.f, 1.f);
            }
        }

        sf::Color c = sf::Color::White;
        c.a = static_cast<sf::Uint8>(alpha * 255.f);
        entity.getComponent<xy::Sprite>().setColour(c);
    }
}

void SimpleShadowSystem::addLight(xy::Entity entity)
{
    XY_ASSERT(entity.hasComponent<Torchlight>(), "Invalid ent");
    m_lights.push_back(entity);
}