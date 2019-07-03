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

#include "NinjaDirector.hpp"
#include "MessageIDs.hpp"
#include "GameConsts.hpp"
#include "NinjaStarSystem.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Callback.hpp>


NinjaDirector::NinjaDirector(SpriteArray<SpriteID::GearBoy::Count>& sa)
    : m_sprites     (sa),
    m_spriteScale   (1.f)
{

}

//public
void NinjaDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        switch (data.type)
        {
        default: break;
        case PlayerEvent::Shot:
            spawnStar(data.entity);
            break;
        }
    }
    else if (msg.id == MessageID::StarMessage)
    {
        const auto& data = msg.getData<StarEvent>();
        switch (data.type)
        {
        default: break;
        case StarEvent::Despawned:
            spawnPuff(data.position);
            break;
        }
    }
}

//private
void NinjaDirector::spawnStar(xy::Entity entity)
{
    auto starEnt = getScene().createEntity();
    starEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getPosition() + GameConst::Gearboy::StarOffset);
    starEnt.getComponent<xy::Transform>().setScale(m_spriteScale, m_spriteScale);
    starEnt.addComponent<xy::Drawable>();
    starEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Star];
    auto bounds = starEnt.getComponent<xy::Sprite>().getTextureBounds();
    starEnt.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    starEnt.addComponent<xy::SpriteAnimation>().play(0);
    starEnt.addComponent<NinjaStar>().velocity.x = entity.getComponent<xy::Transform>().getScale().x * GameConst::Gearboy::StarSpeed;

}

void NinjaDirector::spawnPuff(sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(m_spriteScale, m_spriteScale);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::SmokePuff];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::SpriteAnimation>().play(0);
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