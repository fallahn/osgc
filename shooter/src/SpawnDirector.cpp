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
#include "CollisionTypes.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/util/Random.hpp>

namespace
{
    const sf::Time AmmoRespawn = sf::seconds(12.f);
    const sf::Time BatteryRespawn = sf::seconds(12.f);

    const std::size_t MaxAmmo = 3;
    const std::size_t MaxBattery = 2;

    template <typename T>
    sf::Rect<T>& operator *= (sf::Rect<T>& l, T r)
    {
        l.left *= r;
        l.top *= r;
        l.width *= r;
        l.height *= r;
        return l;
    }
}

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
    else if (msg.id == MessageID::DroneMessage)
    {
        const auto& data = msg.getData<DroneEvent>();
        if (data.type == DroneEvent::Died)
        {
            spawnMiniExplosion(data.position);
        }
    }
}

void SpawnDirector::process(float)
{
    //check to spawn ammo
    if (m_itemClocks[Ammo].getElapsedTime() > AmmoRespawn)
    {
        m_itemClocks[Ammo].restart();

        if (m_activeItems[Ammo] < MaxAmmo)
        {
            m_activeItems[Ammo]++;
            spawnAmmo(getRandomPosition());
        }
    }

    //check to spawn battery
    if (m_itemClocks[Battery].getElapsedTime() > BatteryRespawn)
    {
        m_itemClocks[Battery].restart();

        if (m_activeItems[Battery] < MaxAmmo)
        {
            m_activeItems[Battery]++;
            spawnBattery(getRandomPosition());
        }
    }
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

void SpawnDirector::spawnAmmo(sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::AmmoTop];

    auto bounds = m_sprites[SpriteID::AmmoTop].getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
    
    bounds *= 4.f;
    entity.addComponent<CollisionBox>().worldBounds = { -bounds.width / 2.f, -bounds.height / 2.f, bounds.width, bounds.height };
    entity.getComponent<CollisionBox>().type = CollisionBox::Ammo;

    //radar view
    auto sideEnt = getScene().createEntity();
    bounds = m_sprites[SpriteID::AmmoIcon].getTextureBounds();
    sideEnt.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x + (position.y / 2.f), ConstVal::MaxDroneHeight);
    sideEnt.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);
    sideEnt.getComponent<xy::Transform>().setScale(4.f, 4.f);
    sideEnt.addComponent<xy::Drawable>();
    sideEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::AmmoIcon];
    sideEnt.addComponent<xy::Callback>().active = true;
    sideEnt.getComponent<xy::Callback>().function =
        [&, entity](xy::Entity e, float)
    {
        if (entity.destroyed())
        {
            getScene().destroyEntity(e);
            m_activeItems[Ammo]--;
        }
    };
}

void SpawnDirector::spawnBattery(sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::BatteryTop];

    auto bounds = m_sprites[SpriteID::BatteryTop].getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);

    bounds *= 4.f;
    entity.addComponent<CollisionBox>().worldBounds = { -bounds.width / 2.f, -bounds.height / 2.f, bounds.width, bounds.height };
    entity.getComponent<CollisionBox>().type = CollisionBox::Battery;

    //radar view
    auto sideEnt = getScene().createEntity();
    bounds = m_sprites[SpriteID::BatteryIcon].getTextureBounds();
    sideEnt.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x + (position.y / 2.f), ConstVal::MaxDroneHeight);
    sideEnt.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);
    sideEnt.getComponent<xy::Transform>().setScale(4.f, 4.f);
    sideEnt.addComponent<xy::Drawable>();
    sideEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::BatteryIcon];
    sideEnt.addComponent<xy::Callback>().active = true;
    sideEnt.getComponent<xy::Callback>().function =
        [&, entity](xy::Entity e, float)
    {
        if (entity.destroyed())
        {
            getScene().destroyEntity(e);
            m_activeItems[Battery]--;
        }
    };
}

sf::Vector2f SpawnDirector::getRandomPosition()
{
    static int depth = 1;
    static const int MaxDepth = 8;

    sf::Vector2i randPos(xy::Util::Random::value(0, static_cast<std::int32_t>(ConstVal::MapArea.width)), xy::Util::Random::value(0, static_cast<std::int32_t>(ConstVal::MapArea.height)));
    sf::Vector2f position(randPos);

    sf::FloatRect query(position.x - 12.f, position.y - 12.f, 24.f, 24.f);
    auto nearby = getScene().getSystem<xy::DynamicTreeSystem>().query(query);
    for (auto e : nearby)
    {
        auto bounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
        if (bounds.intersects(query)
            && depth < MaxDepth)
        {
            position = getRandomPosition();
            depth++;
            break;
        }
    }
    depth--;
    return position;
}