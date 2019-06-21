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

#include "VFXDirector.hpp"
#include "ResourceIDs.hpp"
#include "GameConsts.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

VFXDirector::VFXDirector(const SpriteArray& sprites, xy::ResourceHandler& rh)
    : m_sprites(sprites)
{
    m_particles[ParticleID::Win].loadFromFile("assets/particles/success.xyp", rh);
}

//public
void VFXDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::VehicleMessage)
    {
        const auto& data = msg.getData<VehicleEvent>();
        switch (data.type)
        {
        default: break;
        case VehicleEvent::Exploded:
        case VehicleEvent::Eliminated:
            spawnAnimatedSprite(SpriteID::Game::Explosion, data.entity.getComponent<xy::Transform>().getPosition());
            break;
        }
    }
    else if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        switch (data.type)
        {
        default: break;
        case GameEvent::PlayerScored:
        case GameEvent::NewBestTime:
            spawnParticles(ParticleID::Win, data.position);
            break;
        }
    }
}

//private
void VFXDirector::spawnParticles(std::int32_t id, sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.addComponent<xy::ParticleEmitter>().settings = m_particles[id];
    entity.getComponent<xy::ParticleEmitter>().start();
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float)
    {
        if (e.getComponent<xy::ParticleEmitter>().stopped())
        {
            getScene().destroyEntity(e);
        }
    };
}

void VFXDirector::spawnAnimatedSprite(std::int32_t id, sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth + 1);
    entity.addComponent<xy::Sprite>() = m_sprites[id];
    entity.addComponent<xy::SpriteAnimation>().play(0);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float)
    {
        if (e.getComponent<xy::SpriteAnimation>().stopped())
        {
            getScene().destroyEntity(e);
        }
    };
}