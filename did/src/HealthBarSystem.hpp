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

#include "InventorySystem.hpp"

#include <xyginext/ecs/System.hpp>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>

struct HealthBar final
{
    sf::Int8 health = Inventory::MaxHealth;
    sf::Int8 lastHealth = 0;
    sf::Color colour = sf::Color::Red;
    sf::Vector2f size = { 300.f, 46.f };
    float outlineThickness = 6.f;
    xy::Entity parent;
};

class HealthBarSystem final : public xy::System
{
public:
    explicit HealthBarSystem(xy::MessageBus&);

    void process(float) override;

private:

};