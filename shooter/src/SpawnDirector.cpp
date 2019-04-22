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

#include "SpawnDirector.hpp"
#include "MessageIDs.hpp"
#include "GameConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/Scene.hpp>

SpawnDirector::SpawnDirector(SpriteArray& sa)
    : m_sprites(sa)
{

}

//public
void SpawnDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::BombMessage)
    {
        const auto& data = msg.getData<BombEvent>();
        if (data.type == BombEvent::Exploded)
        {
            spawnExplosion(data.position);
            spawnMiniExplosion(data.position);
        }
    }
}

void SpawnDirector::process(float)
{

}

//private
void SpawnDirector::spawnExplosion(sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Explosion];
    entity.addComponent<xy::SpriteAnimation>().play(0);

    auto bounds = m_sprites[SpriteID::Explosion].getTextureBounds();
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

void SpawnDirector::spawnMiniExplosion(sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x + (position.y / 2.f), 308.f);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::ExplosionIcon];
    entity.addComponent<xy::SpriteAnimation>().play(0);

    auto bounds = m_sprites[SpriteID::ExplosionIcon].getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);

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