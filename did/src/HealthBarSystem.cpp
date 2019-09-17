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

#include "HealthBarSystem.hpp"
#include "InventorySystem.hpp"

#include <xyginext/ecs/components/Drawable.hpp>

namespace
{
    const float MaxHealth = static_cast<float>(Inventory::MaxHealth);
}

HealthBarSystem::HealthBarSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(HealthBarSystem))
{
    requireComponent<xy::Drawable>();
    requireComponent<HealthBar>();
}

//public
void HealthBarSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& drawable = entity.getComponent<xy::Drawable>();
        auto& healthBar = entity.getComponent<HealthBar>();

        if (healthBar.lastHealth != healthBar.health)
        {
            auto& verts = drawable.getVertices();

            //hide when no health and parented to actor (but not on UI)
            if (healthBar.health <= 0 && healthBar.parent.isValid())
            {
                verts.clear();
            }
            else
            {
                verts.resize(8);

                float health = healthBar.size.x / MaxHealth;
                health *= healthBar.health;
                health = std::max(0.f, health - healthBar.outlineThickness);

                verts[0] = { sf::Vector2f(), sf::Color::Black };
                verts[1] = { sf::Vector2f(healthBar.size.x, 0.f), sf::Color::Black };
                verts[2] = { healthBar.size, sf::Color::Black };
                verts[3] = { sf::Vector2f(0.f, healthBar.size.y), sf::Color::Black };

                verts[4] = { sf::Vector2f(healthBar.outlineThickness, healthBar.outlineThickness), healthBar.colour };
                verts[5] = { sf::Vector2f(health, healthBar.outlineThickness), healthBar.colour };
                verts[6] = { sf::Vector2f(health, healthBar.size.y - healthBar.outlineThickness), healthBar.colour };
                verts[7] = { sf::Vector2f(healthBar.outlineThickness, healthBar.size.y - healthBar.outlineThickness), healthBar.colour };

                for (auto& v : verts) v.texCoords = v.position;
            }

            healthBar.lastHealth = healthBar.health;
            drawable.updateLocalBounds();
        }        
    }
}