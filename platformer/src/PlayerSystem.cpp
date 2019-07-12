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

#include "Player.hpp"
#include "InputBinding.hpp"
#include "Collision.hpp"
#include "MessageIDs.hpp"
#include "CommandIDs.hpp"
#include "EnemySystem.hpp"
#include "SharedStateData.hpp"

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    sf::FloatRect boundsToWorldspace(sf::FloatRect bounds, const xy::Transform& tx)
    {
        auto scale = tx.getScale();
        bounds.left *= scale.x;
        bounds.width *= scale.x;
        bounds.top *= scale.y;
        bounds.height *= scale.y;
        bounds.left += tx.getPosition().x;
        bounds.top += tx.getPosition().y;

        return bounds;
    }

    const float Gravity = 5999.9f;
    const float JumpImpulse = 1500.f;
    //const float MaxJump = -1500.f;
}

PlayerSystem::PlayerSystem(xy::MessageBus& mb, SharedData& sd)
    : xy::System(mb, typeid(PlayerSystem)),
    m_sharedData(sd)
{
    requireComponent<xy::Transform>();
    requireComponent<xy::BroadphaseComponent>();
    requireComponent<CollisionBody>();
    requireComponent<Player>();
}

//public
void PlayerSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& player = entity.getComponent<Player>();
        auto pos = entity.getComponent<xy::Transform>().getPosition();
        if (!m_bounds.contains(pos) &&
            (player.state == Player::Falling || player.state == Player::Running))
        {
            kill(entity);
        }


        switch (player.state)
        {
        default: break;
        case Player::Falling:
            processFalling(entity, dt);
            DPRINT("State", "Falling");
            break;
        case Player::Running:
            processRunning(entity, dt);
            DPRINT("State", "Running");
            break;
        case Player::Dying:
            processDying(entity, dt);
            DPRINT("State", "Dying");
            break;
        case Player::Dead:
            processDead(entity, dt);
            DPRINT("State", "Dead");
            break;
        }
    }
}

//private
void PlayerSystem::processFalling(xy::Entity entity, float dt)
{
    //parse input
    auto& player = entity.getComponent<Player>();
    auto& tx = entity.getComponent<xy::Transform>();
    auto scale = tx.getScale();
    const auto& collision = entity.getComponent<CollisionBody>();
    if ((player.input & InputFlag::Left)
        && (collision.collisionFlags & CollisionShape::LeftHand) == 0)
    {
        player.velocity.x -= Player::Acceleration * player.accelerationMultiplier;
        scale.x = -scale.y;
    }

    if ((player.input & InputFlag::Right)
        && (collision.collisionFlags & CollisionShape::RightHand) == 0)
    {
        player.velocity.x += Player::Acceleration * player.accelerationMultiplier;
        
        if ((player.input & InputFlag::Left) == 0)
        {
            scale.x = scale.y;
        }
    }
    tx.setScale(scale);

    if((player.prevInput & InputFlag::Jump)
        && (player.input & InputFlag::Jump) == 0)
    {
        player.velocity.y *= 0.3f;
    }
    //walljump?
    /*else if ((player.prevInput & InputFlag::Jump) == 0
        && (player.input & InputFlag::Jump))
    {
        if (collision.collisionFlags & (CollisionShape::LeftHand | CollisionShape::RightHand))
        {
            player.velocity.y -= JumpImpulse;
        }
    }*/

    if ((player.input & InputFlag::Shoot)
        && (player.prevInput & InputFlag::Shoot) == 0)
    {
        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
        msg->type = PlayerEvent::Shot;
        msg->entity = entity;
    }

    player.prevInput = player.input;


    //apply gravity
    float multiplier = (entity.getComponent<Player>().velocity.y < 0) ? 0.7f : 1.2f;
    entity.getComponent<Player>().velocity.y += Gravity * multiplier * dt;

    applyVelocity(entity, dt);
    doCollision(entity, dt);

    //check collision flags and change state if necessary
    if (player.state != Player::Dying) //collision may have killed player
    {
        if (entity.getComponent<CollisionBody>().collisionFlags & (CollisionShape::Player | CollisionShape::Foot))
        {
            entity.getComponent<Player>().state = Player::Running;
            entity.getComponent<xy::SpriteAnimation>().play(player.animations[AnimID::Player::Idle]);

            auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
            msg->type = PlayerEvent::Landed;
            msg->entity = entity;
        }

        //decrease the state time (used for invincibility in this state)
        player.stateTime -= dt;

        if (player.stateTime > 0)
        {
            //flash
            auto& sprite = entity.getComponent<xy::Sprite>();
            auto c = sprite.getColour();
            c.a = (c.a == 0) ? 1 : (c.a == 1) ? 254 : (c.a == 254) ? 255 : (c.a == 255) ? 0 : 0;
            sprite.setColour(c);
        }
        else
        {
            entity.getComponent<xy::Sprite>().setColour(sf::Color::White);
        }
    }
}

void PlayerSystem::processRunning(xy::Entity entity, float dt)
{
    //parse input
    auto& player = entity.getComponent<Player>();
    auto& tx = entity.getComponent<xy::Transform>();
    auto scale = tx.getScale();
    if ((player.input & InputFlag::Left))
    {
        player.velocity.x -= Player::Acceleration * player.accelerationMultiplier;
        scale.x = -scale.y;
    }

    if ((player.input & InputFlag::Right))
    {
        player.velocity.x += Player::Acceleration * player.accelerationMultiplier;
        
        if ((player.input & InputFlag::Left) == 0)
        {
            scale.x = scale.y;
        }
    }
    tx.setScale(scale);

    if ((player.input & InputFlag::Jump)
        && (player.prevInput & InputFlag::Jump) == 0)
    {
        player.velocity.y -= JumpImpulse;
        player.state = Player::Falling;

        entity.getComponent<xy::SpriteAnimation>().play(2);

        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
        msg->type = PlayerEvent::Jumped;
        msg->entity = entity;
    }

    if ((player.input & InputFlag::Shoot)
        && (player.prevInput & InputFlag::Shoot) == 0)
    {
        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
        msg->type = PlayerEvent::Shot;
        msg->entity = entity;
    }

    player.prevInput = player.input;

    //apply the state
    applyVelocity(entity, dt);

    //update the animation based on velocity
    float len2 = xy::Util::Vector::lengthSquared(player.velocity);
    const float MaxVel = 203000.f;

    if (len2 > 10.f)
    {
        if (entity.getComponent<xy::SpriteAnimation>().getAnimationIndex() == player.animations[AnimID::Player::Idle])
        {
            entity.getComponent<xy::SpriteAnimation>().play(player.animations[AnimID::Player::Run]);
        }

        float animSpeed = std::min((len2 / MaxVel), 1.f);
        entity.getComponent<xy::Sprite>().getAnimations()[player.animations[AnimID::Player::Run]].framerate = (14.f * animSpeed) + 6.f;
    }
    else
    {
        //idle
        if (entity.getComponent<xy::SpriteAnimation>().getAnimationIndex() != player.animations[AnimID::Player::Idle])
        {
            entity.getComponent<xy::SpriteAnimation>().play(player.animations[AnimID::Player::Idle]);
        }
    }

    doCollision(entity, dt);

    if (player.state != Player::Dying)//collision may have killed player
    {
        if ((entity.getComponent<CollisionBody>().collisionFlags & CollisionShape::Foot) == 0)
        {
            entity.getComponent<Player>().state = Player::Falling;
            entity.getComponent<xy::SpriteAnimation>().play(player.animations[AnimID::Player::Jump]);
        }
    
        //decrease the state time (used for invincibility in this state)
        player.stateTime -= dt;

        if (player.stateTime > 0)
        {
            //flash
            auto& sprite = entity.getComponent<xy::Sprite>();
            auto c = sprite.getColour();
            c.a = (c.a == 0) ? 1 : (c.a == 1) ? 254 : (c.a == 254) ? 255 : (c.a == 255) ? 0 : 0;
            sprite.setColour(c);
        }
        else
        {
            entity.getComponent<xy::Sprite>().setColour(sf::Color::White);
        }    
    }
}

void PlayerSystem::processDying(xy::Entity entity, float dt)
{
    //gravity
    auto& player = entity.getComponent<Player>();
    player.velocity.y += Gravity * dt;

    //state time
    player.stateTime -= dt;
    if (player.stateTime < 0)
    {
        player.stateTime = Player::DeadTime;
        player.state = Player::Dead;

        entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
    }

    applyVelocity(entity, dt);

    //reset colour just in case
    entity.getComponent<xy::Sprite>().setColour(sf::Color::White);
}

void PlayerSystem::processDead(xy::Entity entity, float dt)
{
    auto& player = entity.getComponent<Player>();
    player.stateTime -= dt;

    if (player.stateTime < 0
        && m_sharedData.inventory.lives > 0)
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::World::CheckPoint;
        cmd.action = [&, entity](xy::Entity e, float) mutable
        {
            auto& p = entity.getComponent<Player>();
            if (p.lastCheckpoint == e.getComponent<CollisionBody>().shapes[0].ID)
            {
                auto bounds = e.getComponent<xy::BroadphaseComponent>().getArea();
                auto position = e.getComponent<xy::Transform>().getPosition();

                p.state = Player::Falling;
                entity.getComponent<xy::Transform>().setPosition(position.x + (bounds.width / 2.f), position.y + (bounds.height / 2.f));
                entity.getComponent<xy::Sprite>().setColour(sf::Color::White);

                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->type = PlayerEvent::Respawned;
                msg->entity = entity;
            }
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

        entity.getComponent<xy::SpriteAnimation>().play(player.animations[AnimID::Player::Jump]);
    }
}

void PlayerSystem::doCollision(xy::Entity entity, float)
{
    auto& tx = entity.getComponent<xy::Transform>();
    entity.getComponent<CollisionBody>().collisionFlags = 0;

    //broad phase
    auto area = entity.getComponent<xy::BroadphaseComponent>().getArea();

    auto queryArea = boundsToWorldspace(area, tx);
    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryArea, CollisionGroup::PlayerFlags);

    for (auto other : nearby)
    {
        if (other != entity)
        {
            auto otherBounds = boundsToWorldspace(other.getComponent<xy::BroadphaseComponent>().getArea(), other.getComponent<xy::Transform>());

            if (otherBounds.intersects(queryArea))
            {
                resolveCollision(entity, other, otherBounds);
            }
        }
    }
}

void PlayerSystem::resolveCollision(xy::Entity entity, xy::Entity other, sf::FloatRect otherBounds)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& collisionBody = entity.getComponent<CollisionBody>();
    auto& player = entity.getComponent<Player>();
    
    //in theory we need to test all the shapes in the other entity, not its global
    //AABB, but for now assume we're only colliding with platforms
    for (auto i = 0u; i < collisionBody.shapeCount; ++i)
    {
        auto bounds = collisionBody.shapes[i].aabb;
        bounds = boundsToWorldspace(bounds, tx);

        if (auto manifold = intersectsAABB(bounds, otherBounds); manifold)
        {
            auto& otherBody = other.getComponent<CollisionBody>();

            //registering foot/hand collisions should only affect state
            //under certain collisions
            if (otherBody.shapes[0].type & (CollisionShape::Solid/* | CollisionShape::Enemy*/))
            {
                collisionBody.collisionFlags |= collisionBody.shapes[i].type;
            }

            if (collisionBody.shapes[i].type == CollisionShape::Player)
            {
                switch (otherBody.shapes[0].type)
                {
                default: break;
                case CollisionShape::Solid:
                    tx.move(manifold->normal * manifold->penetration);

                    if (manifold->normal.y != 0)
                    {
                        player.velocity = {};
                    }
                    else
                    {
                        player.velocity = xy::Util::Vector::reflect(player.velocity, manifold->normal);
                    }
                    break;
                case CollisionShape::Fluid:
                case CollisionShape::Spikes:
                    tx.move(manifold->normal * manifold->penetration);
                    if (player.stateTime < 0) //used for invincibility
                    {
                        kill(entity);
                    }
                    else
                    {
                        player.velocity = xy::Util::Vector::reflect(player.velocity, manifold->normal);
                    }
                    return;
                case CollisionShape::Checkpoint:
                    //we can assume we're alive at this point because
                    //dead/dying states perform no collision
                    player.lastCheckpoint = otherBody.shapes[0].ID;

                    //trigger the animation on the checkpoint
                    if (player.lastCheckpoint > 0)
                    {
                        other.getComponent<xy::Callback>().active = true;
                    }
                    break;
                case CollisionShape::Exit:
                {
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->type = PlayerEvent::Exited;
                    msg->entity = entity;
                    msg->id = otherBody.shapes[0].ID;

                    player.state = Player::Dead; //just prevents further collisions
                    return;
                }
                case CollisionShape::Collectible:
                {
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->entity = entity;

                    switch (otherBody.shapes[0].ID)
                    {
                    default:
                    case 0:
                        msg->type = PlayerEvent::GotCoin;
                        break;
                    case 1:
                        msg->type = PlayerEvent::GotShield;
                        player.hasShield = true;
                        break;
                    case 2:
                        msg->type = PlayerEvent::GotAmmo;
                        break;
                    }

                    getScene()->destroyEntity(other);
                }
                    break;
                case CollisionShape::Enemy:
                    if (other.getComponent<Enemy>().state == Enemy::Normal)
                    {
                        tx.move(manifold->normal * manifold->penetration);
                        if (manifold->normal.y < 0)
                        {
                            //we landed on top, so kill enemy
                            player.velocity = xy::Util::Vector::reflect(player.velocity, manifold->normal) * 0.5f;

                            //TODO should rely on sending a player action message
                            //and allow enemy system to kill enemy instead
                            auto* msg = postMessage<EnemyEvent>(MessageID::EnemyMessage);
                            msg->entity = other;
                            msg->type = EnemyEvent::Died;

                            other.getComponent<Enemy>().kill();
                        }
                        else if (player.stateTime < 0) //state time is used for invincibility
                        {
                            kill(entity);
                            return;
                        }
                    }
                    break;
                case CollisionShape::Dialogue:
                {
                    auto& dialogue = other.getComponent<Dialogue>();
                    if (!dialogue.expired)
                    {
                        m_sharedData.dialogueFile = dialogue.file;
                        dialogue.expired = true;

                        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                        msg->type = PlayerEvent::TriggerDialogue;
                        msg->entity = entity;
                    }
                }
                    break;
                }
            }
        }
    }
}

void PlayerSystem::applyVelocity(xy::Entity entity, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& player = entity.getComponent<Player>();

    tx.move(player.velocity * dt);
    player.velocity.x *= Player::Drag;
}

void PlayerSystem::kill(xy::Entity entity)
{
    auto& player = entity.getComponent<Player>();

    if (player.hasShield)
    {
        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
        msg->type = PlayerEvent::LostShield;
        msg->entity = entity;

        player.hasShield = false;
        player.velocity = -player.velocity * 0.25f;
        player.stateTime = Player::InvincibleTime;
    }
    else
    {
        player.velocity.y = -1100.f;
        player.velocity.x = 200.f;
        player.state = Player::Dying;
        player.stateTime = Player::DyingTime;

        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
        msg->type = PlayerEvent::Died;
        msg->entity = entity;

        entity.getComponent<xy::SpriteAnimation>().play(player.animations[AnimID::Player::Die]);
    }
}