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

#include "EnemySystem.hpp"
#include "MessageIDs.hpp"
#include "Collision.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float Gravity = 3600.f;

    float easeInOutQuad(float t)
    {
        return t < 0.5f ? 2.f * t * t : t * (4.f - 2.f * t) - 1.f;
    }
}

EnemySystem::EnemySystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(EnemySystem))
{
    requireComponent<Enemy>();
    requireComponent<xy::Transform>();
}

//public
void EnemySystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::StarMessage)
    {
        const auto& data = msg.getData<StarEvent>();
        if (data.type == StarEvent::HitItem)
        {
            if (data.collisionShape & CollisionShape::Enemy)
            {
                auto entity = data.entityHit;
                kill(entity);

                auto* msg2 = postMessage<EnemyEvent>(MessageID::EnemyMessage);
                msg2->type = EnemyEvent::Died;
                msg2->entity = entity;
            }
        }
    }
    else if (msg.id == MessageID::CrateMessage)
    {
        const auto& data = msg.getData<CrateEvent>();
        if (data.type == CrateEvent::KilledEnemy)
        {
            auto entity = data.entityHit;
            kill(entity);

            auto* msg2 = postMessage<EnemyEvent>(MessageID::EnemyMessage);
            msg2->type = EnemyEvent::Died;
            msg2->entity = entity;
        }
    }
}

void EnemySystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& enemy = entity.getComponent<Enemy>();
        auto& tx = entity.getComponent<xy::Transform>();

        //update sprite direction
        if (enemy.type != Enemy::Rocket)
        {
            if ((enemy.velocity.x < 0 && tx.getScale().x < 0)
                || (enemy.velocity.x > 0 && tx.getScale().x > 0))
            {
                tx.scale(-1.f, 1.f);
            }
        }
        
        if (enemy.state == Enemy::Normal)
        {
            switch (enemy.type)
            {
            default: break;
            case Enemy::Bird:
                processBird(entity, dt);
                break;
            case Enemy::Crawler:
                processCrawler(entity, dt);
                break;
            case Enemy::Walker:
                processWalker(entity, dt);
                break;
            case Enemy::Egg:
                processEgg(entity, dt);
                break;
            case Enemy::Orb:
                processOrb(entity, dt);
                break;
            case Enemy::Spitball:
                processSpitball(entity, dt);
                break;
            case Enemy::Rocket:
                processRocket(entity, dt);
                break;
            }
        }
        else
        {
            processDying(entity, dt);
        }
    }
}

//private
void EnemySystem::processCrawler(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();

    //turn around at end of path - TODO refactor into single func
    tx.move(enemy.velocity * dt);
    if (tx.getPosition().x > enemy.end.x)
    {
        enemy.velocity.x = -enemy.velocity.x;
        tx.setPosition(enemy.end);
    }
    else if (tx.getPosition().x < enemy.start.x)
    {
        enemy.velocity.x = -enemy.velocity.x;
        tx.setPosition(enemy.start);
    }

    //randomly pause
    enemy.stateTime += dt;
    if (enemy.stateTime > enemy.targetTime)
    {
        enemy.stateTime = 0.f;
        enemy.targetTime = xy::Util::Random::value(1.5f, 3.5f);

        if (enemy.velocity.x == 0)
        {
            enemy.velocity.x = (xy::Util::Random::value(0, 1) == 0) ? -Enemy::CrawlerVelocity : Enemy::CrawlerVelocity;
            entity.getComponent<xy::SpriteAnimation>().play(enemy.animationIDs[AnimID::Enemy::Walk]);
        }
        else
        {
            enemy.velocity.x = 0.f;
            entity.getComponent<xy::SpriteAnimation>().play(enemy.animationIDs[AnimID::Enemy::Idle]);
        }
    }

    doCollision(entity);
}

void EnemySystem::processBird(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();

    //turn around at end of path
    tx.move(enemy.velocity * dt);
    if (tx.getPosition().x > enemy.end.x)
    {
        enemy.velocity.x = -enemy.velocity.x;
        tx.setPosition(enemy.end);
    }
    else if (tx.getPosition().x < enemy.start.x)
    {
        enemy.velocity.x = -enemy.velocity.x;
        tx.setPosition(enemy.start);
    }
    
    //drop occasional egg
    enemy.stateTime += dt;
    if (enemy.stateTime > enemy.targetTime)
    {
        enemy.stateTime = 0.f;
        enemy.targetTime = xy::Util::Random::value(6.f, 8.f);

        //only drop egg is bird is visible
        auto pos = getScene()->getActiveCamera().getComponent<xy::Transform>().getPosition();
        pos -= xy::DefaultSceneSize / 2.f;
        
        sf::FloatRect view(pos, xy::DefaultSceneSize);

        if (view.contains(tx.getPosition()))
        {
            auto* msg = postMessage<EnemyEvent>(MessageID::EnemyMessage);
            msg->type = EnemyEvent::SpawnEgg;
            msg->entity = entity;
        }
    }
}

void EnemySystem::processWalker(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();

    //turn around at end of path
    tx.move(enemy.velocity * dt);
    if (tx.getPosition().x > enemy.end.x)
    {
        enemy.velocity.x = -enemy.velocity.x;
        tx.setPosition(enemy.end);
    }
    else if (tx.getPosition().x < enemy.start.x)
    {
        enemy.velocity.x = -enemy.velocity.x;
        tx.setPosition(enemy.start);
    }

    doCollision(entity);

    //TODO stop and shoot
}

void EnemySystem::processDying(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();

    enemy.velocity.y += Gravity * dt;
    tx.move(enemy.velocity * dt);

    enemy.stateTime += dt;

    if (enemy.stateTime > Enemy::DyingTime)
    {
        getScene()->destroyEntity(entity);
    }

    //death animations if available on current sprite
    if (entity.getComponent<xy::Sprite>().getAnimationCount() > 1
        && entity.getComponent<xy::SpriteAnimation>().getAnimationIndex() == 0)
    {
        entity.getComponent<xy::SpriteAnimation>().play(1);
    }
}

void EnemySystem::processEgg(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();

    enemy.velocity.y += (Gravity / 2.f) * dt;
    tx.move(enemy.velocity * dt);

    enemy.stateTime += dt;

    if (enemy.stateTime > Enemy::EggLife)
    {
        //getScene()->destroyEntity(entity);

        enemy.kill();

        auto* msg = postMessage<EnemyEvent>(MessageID::EnemyMessage);
        msg->type = EnemyEvent::Died;
        msg->entity = entity;
    }
}

void EnemySystem::processOrb(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();

    //only using velocity to track direction - doesn't
    //influence the actual speed
    if (enemy.velocity.y > 0)
    {
        enemy.stateTime = std::min(1.f, enemy.stateTime + dt);
        if (enemy.stateTime == 1)
        {
            enemy.velocity.y = -enemy.velocity.y;
        }
    }
    else
    {
        enemy.stateTime = std::max(0.f, enemy.stateTime - dt);
        if (enemy.stateTime == 0)
        {
            enemy.velocity.y = -enemy.velocity.y;
        }
    }

    auto pathDist = enemy.end - enemy.start;
    sf::Vector2f pathPos = easeInOutQuad(enemy.stateTime) * pathDist;

    tx.setPosition(enemy.start + pathPos);
}

void EnemySystem::processSpitball(xy::Entity entity, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& enemy = entity.getComponent<Enemy>();


    if (tx.getPosition().y <= enemy.start.y)
    {
        //currently moving
        tx.move(enemy.velocity * dt);
        enemy.velocity.y += Gravity * dt;

        auto scale = tx.getScale();
        if (enemy.velocity.y < 0)
        {
            scale.y = -scale.x;
        }
        else
        {
            scale.y = scale.x;
        }
        tx.setScale(scale);
    }
    else
    {
        //pause for some time
        enemy.stateTime -= dt;
        if (enemy.stateTime < 0)
        {
            enemy.stateTime = 1.f;
            enemy.velocity = { 0.f, -(1600.f + xy::Util::Random::value(0.f, 100.f)) };

            //kind of a kludge to make spitballs jump higher based on the map data
            enemy.velocity.y *= -((enemy.end.y - enemy.start.y) / 448.f); //112 * scale

            tx.setPosition(enemy.start);

            entity.getComponent<xy::AudioEmitter>().play();
        }
    }
}

void EnemySystem::processRocket(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();

    if (enemy.stateTime > 0)
    {
        enemy.stateTime -= dt;

        if (enemy.stateTime < 0)
        {
            //respawn
            auto scale = tx.getScale();
            scale.x = scale.y;
            tx.setScale(scale);

            entity.getComponent<xy::AudioEmitter>().play();

            auto* msg = postMessage<StarEvent>(MessageID::StarMessage);
            msg->position = enemy.start;
            msg->type = StarEvent::None;
        }
    }
    else
    {
        tx.move(enemy.velocity * dt);

        auto maxDist = xy::Util::Vector::lengthSquared(enemy.end - enemy.start);
        auto currDist = xy::Util::Vector::lengthSquared(tx.getPosition() - enemy.start);

        if (currDist > maxDist)
        {
            enemy.stateTime = 3.f;
            auto scale = tx.getScale();
            scale.x = 0.f;
            tx.setScale(scale);
            tx.setPosition(enemy.start);

            auto* msg = postMessage<StarEvent>(MessageID::StarMessage);
            msg->position = enemy.end;
            msg->type = StarEvent::None;
        }
    }
}

void EnemySystem::kill(xy::Entity entity)
{
    entity.getComponent<Enemy>().kill();
    entity.getComponent<xy::Drawable>().bindUniform("u_depth", 40.f);
}

void EnemySystem::doCollision(xy::Entity entity)
{
    //TODO skip this if we know there are no crates on the map

    auto& tx = entity.getComponent<xy::Transform>();

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    auto scale = tx.getScale() / 2.f;
    bounds.left = tx.getPosition().x;
    bounds.top = tx.getPosition().y;
    bounds.left -= tx.getOrigin().x * scale.x;
    bounds.top -= tx.getOrigin().y * scale.y;
    bounds.width *= scale.x;
    bounds.height *= scale.y;

    auto query = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds, CollisionShape::Crate);
    for (auto other : query)
    {
        //if (other != entity)
        {
            const auto& otherTx = other.getComponent<xy::Transform>();

            auto otherBounds = other.getComponent<xy::BroadphaseComponent>().getArea();
            otherBounds = otherTx.getTransform().transformRect(otherBounds);

            auto origin = otherTx.getOrigin() * tx.getScale().y;
            otherBounds.left += origin.x;
            otherBounds.top += origin.y;

            if (otherBounds.intersects(bounds))
            {
                auto manifold = intersectsAABB(bounds, otherBounds);
                tx.move(manifold->normal * manifold->penetration);
                entity.getComponent<Enemy>().velocity.x *= -1.f;

                break;
            }
        }
    }
}

void EnemySystem::onEntityAdded(xy::Entity entity)
{
    auto& enemy = entity.getComponent<Enemy>();
    enemy.velocity.x = (xy::Util::Random::value(0, 1) == 0) ? -1.f : 1.f;

    switch (enemy.type)
    {
    default:
    case Enemy::Bird:
        enemy.velocity.x *= Enemy::BirdVelocity;
        enemy.targetTime = xy::Util::Random::value(4.f, 5.f);
        break;
    case Enemy::Crawler:
        enemy.velocity.x *= Enemy::CrawlerVelocity;
        break;
    case Enemy::Walker:
        enemy.velocity.x *= Enemy::WalkerVelocity;
        break;
    case Enemy::Rocket:
        enemy.velocity = enemy.end - enemy.start;
        enemy.velocity = xy::Util::Vector::normalise(enemy.velocity);
        enemy.velocity *= Enemy::RocketVelocity;
        entity.getComponent<xy::Transform>().setRotation(xy::Util::Vector::rotation(enemy.velocity));
        break;
    case Enemy::Egg:
    case Enemy::Spitball:
        enemy.velocity.x = 0.f;
        break;
    case Enemy::Orb:
        enemy.velocity.y = enemy.velocity.x;
        enemy.velocity.x = 0.f;
        enemy.stateTime = 0.5f;
        break;
    }
}