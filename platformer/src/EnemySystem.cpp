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
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float Gravity = 4000.f;

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
                entity.getComponent<Enemy>().kill();

                auto* msg2 = postMessage<EnemyEvent>(MessageID::EnemyMessage);
                msg2->type = EnemyEvent::Died;
                msg2->entity = entity;
            }
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
        if ((enemy.velocity.x < 0 && tx.getScale().x < 0)
            || (enemy.velocity.x > 0 && tx.getScale().x > 0))
        {
            tx.scale(-1.f, 1.f);
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
            entity.getComponent<xy::SpriteAnimation>().play(0);
        }
        else
        {
            enemy.velocity.x = 0.f;
            entity.getComponent<xy::SpriteAnimation>().pause();
        }
    }
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

    float pathDist = enemy.end.y - enemy.start.y;
    float pathPos = easeInOutQuad(enemy.stateTime) * pathDist;

    auto pos = tx.getPosition();
    pos.y = enemy.start.y + pathPos;
    tx.setPosition(pos);
}

void EnemySystem::processSpitball(xy::Entity entity, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& enemy = entity.getComponent<Enemy>();


    if (tx.getPosition().y < enemy.start.y)
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
            enemy.stateTime = 1.5f;
            enemy.velocity = { 0.f, -(1000.f + xy::Util::Random::value(0.f, 100.f)) };

            tx.setPosition(enemy.start);
        }
    }
}

void EnemySystem::processRocket(xy::Entity entity, float dt)
{
    auto& enemy = entity.getComponent<Enemy>();
    auto& tx = entity.getComponent<xy::Transform>();
    tx.move(enemy.velocity * dt);

    auto maxDist = xy::Util::Vector::lengthSquared(enemy.end - enemy.start);
    auto currDist = xy::Util::Vector::lengthSquared(tx.getPosition() - enemy.start);

    if (currDist > maxDist)
    {
        //TODO raise a message
        getScene()->destroyEntity(entity);
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