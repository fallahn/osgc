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

#include "ResourceIDs.hpp"

#include <xyginext/ecs/Director.hpp>
#include <xyginext/ecs/Entity.hpp>
#include <xyginext/ecs/components/ParticleEmitter.hpp>

#include <SFML/System/Vector2.hpp>

class NinjaDirector final : public xy::Director
{
public:
    NinjaDirector(const SpriteArray<SpriteID::GearBoy::Count>&, const std::array<xy::EmitterSettings, ParticleID::Count>&);

    void handleMessage(const xy::Message&) override;

    void setSpriteScale(float scale) { m_spriteScale = scale; }

private:

    const SpriteArray<SpriteID::GearBoy::Count>& m_sprites;
    const std::array<xy::EmitterSettings, ParticleID::Count>& m_particleEmitters;
    float m_spriteScale;

    void spawnStar(xy::Entity);
    void spawnPuff(sf::Vector2f);
    void spawnEgg(sf::Vector2f);
    void spawnShield(xy::Entity);
};
