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
#include "EnemySystem.hpp"
#include "Collision.hpp"
#include "ShieldAnimationSystem.hpp"
#include "CommandIDs.hpp"
#include "CameraTarget.hpp"
#include "SharedStateData.hpp"
#include "Player.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/detail/Operators.hpp>

NinjaDirector::NinjaDirector(const SpriteArray<SpriteID::GearBoy::Count>& sa, const std::array<xy::EmitterSettings, ParticleID::Count>& es, const SharedData& sd)
    : m_sprites         (sa),
    m_particleEmitters  (es),
    m_sharedData        (sd),
    m_spriteScale       (1.f)
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
        case PlayerEvent::Respawned:
            {
                auto camera = getScene().getActiveCamera();
                if (camera.hasComponent<CameraTarget>())
                {
                    camera.getComponent<CameraTarget>().shakeAmount = 1.f;
                }

                auto entity = data.entity;

                //trigger particles on spawner
                xy::Command cmd;
                cmd.targetFlags = CommandID::World::CheckPoint;
                cmd.action = [&, entity](xy::Entity e, float) mutable
                {
                    auto& p = entity.getComponent<Player>();
                    if (p.lastCheckpoint > 0 &&
                        p.lastCheckpoint == e.getComponent<CollisionBody>().shapes[0].ID)
                    {
                        e.getComponent<xy::Callback>().active = true;
                    }
                };
                sendCommand(cmd);
            }

        //case PlayerEvent::Jumped:
        case PlayerEvent::Landed:
            if (data.entity.isValid())
            {
                //TODO sometimes this is raised at the end of the level
                //and carried over to the next map - in which case the data
                //is invalid. Need to prevent this.
                spawnPuff(data.entity.getComponent<xy::Transform>().getPosition());
            }
            break;
        case PlayerEvent::GotShield:
            spawnShield(data.entity);
            break;
        case PlayerEvent::LostShield:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::World::ShieldParticle;
            cmd.action = [&](xy::Entity e, float)
            {
                getScene().destroyEntity(e);
            };
            sendCommand(cmd);
        }
        break;
        }
    }
    else if (msg.id == MessageID::StarMessage)
    {
        const auto& data = msg.getData<StarEvent>();
        switch (data.type)
        {
        default: break;
        case StarEvent::HitItem:
        case StarEvent::None:
            spawnPuff(data.position);
            break;
        }
    }
    else if (msg.id == MessageID::EnemyMessage)
    {
        const auto& data = msg.getData<EnemyEvent>();
        if (data.type == EnemyEvent::Died)
        {
            auto position = data.entity.getComponent<xy::Transform>().getPosition();
            spawnPuff(position);
        }
        else if (data.type == EnemyEvent::SpawnEgg)
        {
            auto position = data.entity.getComponent<xy::Transform>().getPosition();
            spawnEgg(position);
        }
    }
    else if (msg.id == MessageID::CrateMessage)
    {
        const auto& data = msg.getData<CrateEvent>();
        switch (data.type)
        {
        default: break;
        case CrateEvent::Landed:
            spawnPuff(data.position);
            break;
        }
    }
}

//private
void NinjaDirector::spawnStar(xy::Entity entity)
{
    if (m_sharedData.inventory.ammo > 0)
    {
        const auto& tx = entity.getComponent<xy::Transform>();
        auto position = tx.getWorldPosition() + (tx.getOrigin() * tx.getScale());
        position += GameConst::Gearboy::StarOffset;

        auto starEnt = getScene().createEntity();
        starEnt.addComponent<xy::Transform>().setPosition(position);
        starEnt.addComponent<xy::Drawable>();
        starEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Star];
        auto bounds = starEnt.getComponent<xy::Sprite>().getTextureBounds();
        starEnt.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        starEnt.addComponent<xy::SpriteAnimation>().play(0);
        starEnt.addComponent<NinjaStar>().velocity.x = entity.getComponent<xy::Transform>().getScale().x * GameConst::Gearboy::StarSpeed;

        auto scale = starEnt.getComponent<NinjaStar>().velocity.x > 0 ? m_spriteScale : -m_spriteScale;
        starEnt.getComponent<xy::Transform>().setScale(scale, m_spriteScale);
    }
}

void NinjaDirector::spawnPuff(sf::Vector2f position)
{
    //TODO pool and recycle these entities
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

void NinjaDirector::spawnEgg(sf::Vector2f position)
{
    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(m_spriteScale, m_spriteScale);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Bomb];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<Enemy>().type = Enemy::Egg;

    bounds.left -= bounds.width / 2.f;
    bounds.top -= bounds.height / 2.f;

    auto& collision = entity.addComponent<CollisionBody>();
    collision.shapes[0].aabb = bounds;
    collision.shapes[0].type = CollisionShape::Enemy;
    collision.shapes[0].collisionFlags = CollisionShape::Player;

    entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Enemy);
    entity.addComponent<xy::SpriteAnimation>().play(0);
}

void NinjaDirector::spawnShield(xy::Entity playerEnt)
{
    auto bounds = m_sprites[SpriteID::GearBoy::ShieldAvatar].getTextureBounds();
    float scale = playerEnt.getComponent<xy::Transform>().getScale().y;

    auto entity = getScene().createEntity();
    entity.addComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::ParticleEmitter>().settings = m_particleEmitters[ParticleID::Shield];
    entity.getComponent<xy::ParticleEmitter>().start();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::World::ShieldParticle;
    entity.addComponent<ShieldAnim>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::ShieldAvatar];
    entity.addComponent<xy::SpriteAnimation>().play(0);
    entity.addComponent<xy::Drawable>().setDepth(1);

    auto targetEnt = getScene().createEntity();
    targetEnt.addComponent<xy::Transform>().setOrigin(0.f, 48.f);
    targetEnt.addComponent<CameraTarget>().target = playerEnt;
    targetEnt.addComponent<xy::CommandTarget>().ID = CommandID::World::ShieldParticle;
    targetEnt.addComponent<xy::Camera>(); //dummy camera just so we can take advantage of target component

    targetEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}