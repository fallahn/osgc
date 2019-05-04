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

#include "Drone.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "GameConsts.hpp"
#include "Bomb.hpp"
#include "ItemBar.hpp"
#include "CollisionTypes.hpp"
#include "Alien.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Wavetable.hpp>

namespace
{
    const float Acceleration = 60.f;
    const float MaxVelocity = 800.f;
    const float MaxVelocitySqr = MaxVelocity * MaxVelocity;

    const float DroneDipHeight = 290.f; //dips this low for pickups
    const float Gravity = 9.9f;
}

DroneSystem::DroneSystem(xy::MessageBus& mb, const SpriteArray& sprites, std::int32_t difficulty)
    : xy::System(mb, typeid(DroneSystem)),
    m_sprites   (sprites),
    m_difficulty(static_cast<float>(difficulty))
{
    requireComponent<xy::Transform>();
    requireComponent<Drone>();

    m_wavetable = xy::Util::Wavetable::sine(0.25f, (DroneDipHeight - ConstVal::DroneHeight));
}

//public
void DroneSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::BombMessage)
    {
        const auto& data = msg.getData<BombEvent>();

        //might as well check to see what we damaged
        auto damageArea = ConstVal::DamageRadius;
        damageArea.left += data.position.x;
        damageArea.top += data.position.y;

        if (data.type == BombEvent::Exploded)
        {
            auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(damageArea, CollisionBox::Collectible | CollisionBox::Alien | CollisionBox::Solid | CollisionBox::Human);
            for (auto e : nearby)
            {
                auto eBounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
                if (eBounds.intersects(damageArea))
                {
                    //only raise messages in specific cases else we'll cause a feedback loop!

                    switch (e.getComponent<CollisionBox>().type)
                    {
                    default: break;
                    case CollisionBox::Ammo:
                    case CollisionBox::Battery:
                    {
                        auto* msg = postMessage<BombEvent>(MessageID::BombMessage);
                        msg->position = e.getComponent<xy::Transform>().getPosition();
                        msg->type = BombEvent::DestroyedCollectible;
                        getScene()->destroyEntity(e);
                    }
                        break;
                    case CollisionBox::Building:
                    {
                        auto* msg2 = postMessage<BombEvent>(MessageID::BombMessage);
                        msg2->position = data.position;
                        msg2->type = BombEvent::DamagedBuilding;
                    }
                        break;
                    case CollisionBox::NPC:
                    {
                        auto* msg = postMessage<BombEvent>(MessageID::BombMessage);
                        msg->position = e.getComponent<xy::Transform>().getPosition();
                        if (e.hasComponent<Alien>())
                        {
                            msg->type = e.getComponent<Alien>().type == Alien::Type::Beetle ? BombEvent::KilledBeetle : BombEvent::KilledScorpion;
                        }
                        else
                        {
                            msg->type = BombEvent::KilledHuman;
                        }
                        msg->rotation = e.getComponent<xy::Transform>().getRotation();

                        getScene()->destroyEntity(e);
                    }
                        break;
                    }
                }
            }
        }
    }
}

void DroneSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& drone = entity.getComponent<Drone>();

        switch (drone.state)
        {
        default: break;
        case Drone::State::Flying:
            processFlying(entity, dt);
            break;
        case Drone::State::PickingUp:
            processPickingUp(entity, dt);
            break;
        case Drone::State::Dying:
            processDying(entity, dt);
            break;
        case Drone::State::Dead: return;
        }

        if (drone.health == 0)
        {
            drone.state = Drone::State::Dying;
        }

        //update health meter
        xy::Command cmd;
        cmd.targetFlags = CommandID::HealthMeter;
        cmd.action = [drone](xy::Entity e, float)
        {
            sf::Color c;
            if (drone.health < 34)
            {
                c = sf::Color::Red;
            }
            else if (drone.health < 67)
            {
                c = sf::Color::Yellow;
            }
            else
            {
                c = sf::Color::Green;
            }

            auto& sprite = e.getComponent<xy::Sprite>();
            sprite.setColour(c);
            auto bounds = e.getComponent<sf::FloatRect>();
            bounds.width *= (drone.health / 100.f);
            sprite.setTextureRect(bounds);
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

//private
void DroneSystem::processFlying(xy::Entity entity, float dt)
{
    auto& drone = entity.getComponent<Drone>();

    //calc new velocity from input
    sf::Vector2f velocity;
    if (drone.inputFlags & Drone::Up)
    {
        velocity.y -= 1.f;
    }
    if (drone.inputFlags & Drone::Down)
    {
        velocity.y += 1.f;
    }
    if (drone.inputFlags & Drone::Left)
    {
        velocity.x -= 1.f;
    }
    if (drone.inputFlags & Drone::Right)
    {
        velocity.x += 1.f;
    }

    if (float len2 = xy::Util::Vector::lengthSquared(velocity); len2 > 1)
    {
        float len = std::sqrt(len2);
        velocity /= len;
    }
    drone.velocity += velocity * Acceleration;

    //clamp to max velocity
    if (float len2 = xy::Util::Vector::lengthSquared(drone.velocity); len2 > MaxVelocitySqr)
    {
        drone.velocity *= (MaxVelocitySqr / len2);
    }

    //set position and update side view drone
    auto & tx = entity.getComponent<xy::Transform>();
    auto pos = tx.getPosition();
    pos += drone.velocity * dt;
    pos.x = xy::Util::Math::clamp(pos.x, 64.f, ConstVal::MapArea.width - 128.f);
    pos.y = xy::Util::Math::clamp(pos.y, 64.f, ConstVal::MapArea.height - 128.f);
    tx.setPosition(pos);

    drone.velocity *= 0.9f;

    float sidePos = ConstVal::BackgroundPosition.x + (pos.y / 2.f);

    xy::Command cmd;
    cmd.targetFlags = CommandID::PlayerSide;
    cmd.action = [sidePos](xy::Entity e, float)
    {
        auto& tx = e.getComponent<xy::Transform>();
        auto pos = tx.getPosition();
        pos.x = sidePos;
        tx.setPosition(pos);
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

    //check is fire button pressed and rest input ready for next frame
    if (drone.inputFlags & Drone::Fire)
    {
        if (drone.ammo > 0)
        {
            drone.ammo--;
            spawnBomb(pos, drone.velocity);

            updateAmmoBar(drone);
        }
        else
        {
            entity.getComponent<xy::AudioEmitter>().play();
        }
    }

    if (drone.inputFlags & Drone::Pickup)
    {
        drone.state = Drone::State::PickingUp;
        entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
    }

    drone.inputFlags = 0;

    //slowly drain battery - TODO also in pick up state?
    auto oldBatt = drone.battery;
#ifndef XY_DEBUG
    drone.battery = std::max(0.f, drone.battery - (dt * (3.33f / m_difficulty))); //30 seconds(ish)
#endif // !XY_DEBUG
        
    //send update to battery indicator
    updateBatteryBar(drone);

    //raise event on low battery
    if (oldBatt > 28 && drone.battery < 28)
    {
        auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
        msg->type = DroneEvent::BatteryLow;
        msg->position = tx.getPosition();
    }

    if (drone.battery == 0)
    {
        drone.state = Drone::State::Dying;
        entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);

        auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
        msg->type = DroneEvent::BatteryFlat;
        msg->position = tx.getPosition();
    }
}

void DroneSystem::processPickingUp(xy::Entity entity, float dt)
{
    auto& drone = entity.getComponent<Drone>();
    if (drone.wavetableIndex < m_wavetable.size() / 2)
    {
        auto newHeight = m_wavetable[drone.wavetableIndex++] + ConstVal::DroneHeight;
        auto& tx = entity.getComponent<xy::Transform>();

        //check for collision
        bool collision = false;
        std::int32_t collisionType = CollisionBox::Structure;
        xy::Entity collider;

        auto& bp = getScene()->getSystem<xy::DynamicTreeSystem>();
        auto position = tx.getPosition();
        sf::FloatRect droneBounds = { position.x - 8.f, position.y - 8.f, 16.f, 16.f };

        auto nearby = bp.query(droneBounds, CollisionBox::Solid | CollisionBox::Collectible | CollisionBox::Explosion | CollisionBox::Alien);
        for (auto e : nearby)
        {
            auto otherBounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
            if (otherBounds.intersects(droneBounds)
                && newHeight > (ConstVal::MaxDroneHeight - e.getComponent<CollisionBox>().height))
            {
                collision = true;
                collisionType = e.getComponent<CollisionBox>().type;
                collider = e;
                break;
            }
        }

        if (!collision)
        {
            drone.height = newHeight;

            if (drone.colliding)
            {
                //collision just ended
                auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
                msg->type = DroneEvent::CollisionEnd;
                msg->position = tx.getPosition();
            }
        }
        else
        {
            switch (collisionType)
            {
            case CollisionBox::Fire:
            case CollisionBox::NPC:
                drone.height = newHeight;
            default:
            case CollisionBox::Building:
            case CollisionBox::Structure:
                drone.health = std::max(0.f, drone.health - (dt * 50.f));

                if (!drone.colliding)
                {
                    //collision just started
                    auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
                    msg->type = DroneEvent::CollisionStart;
                    msg->position = tx.getPosition();
                }

                break;
            case CollisionBox::Ammo:
                drone.height = newHeight;
                getScene()->destroyEntity(collider);
                drone.ammo = Drone::StartAmmo;
                updateAmmoBar(drone);
                {
                    auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
                    msg->position = entity.getComponent<xy::Transform>().getPosition();
                    msg->type = DroneEvent::GotAmmo;
                }
                break;
            case CollisionBox::Battery:
                drone.height = newHeight;
                getScene()->destroyEntity(collider);
                drone.battery = Drone::StartBattery;
                updateBatteryBar(drone);
                {
                    auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
                    msg->position = entity.getComponent<xy::Transform>().getPosition();
                    msg->type = DroneEvent::GotBattery;
                }
                break;
            }
        }
        drone.colliding = collision;

        //update top view
        tx.move(drone.velocity * dt);
        auto sidePos = ConstVal::BackgroundPosition.x + (tx.getPosition().y / 2.f);
        drone.camera.getComponent<xy::Camera>().setZoom(((drone.height - ConstVal::DroneHeight) * 0.15f) + 1.f);
        drone.velocity *= 0.9f;

        //update side view
        xy::Command cmd;
        cmd.targetFlags = CommandID::PlayerSide;
        cmd.action = [drone, sidePos](xy::Entity e, float)
        {
            auto& tx = e.getComponent<xy::Transform>();
            auto pos = tx.getPosition();
            pos.x = sidePos;
            pos.y = drone.height;
            tx.setPosition(pos);
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);  
    }
    else
    {
        //reset drone
        drone.reset();
        entity.getComponent<xy::Sprite>().setColour(sf::Color::White);

        xy::Command cmd;
        cmd.targetFlags = CommandID::PlayerSide;
        cmd.action = [](xy::Entity e, float)
        {
            auto& tx = e.getComponent<xy::Transform>();
            auto pos = tx.getPosition();
            pos.y = ConstVal::DroneHeight;
            tx.setPosition(pos);
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void DroneSystem::processDying(xy::Entity entity, float dt)
{
    //fall to the floor, raise message when hitting the ground
    //and subtract life
    auto& drone = entity.getComponent<Drone>();
    drone.gravity += Gravity;
    drone.height += drone.gravity * dt;
    
    //update top view
    auto& tx = entity.getComponent<xy::Transform>();
    tx.move(drone.velocity * dt);
    auto sidePos = ConstVal::BackgroundPosition.x + (tx.getPosition().y / 2.f);

    float zoom = ((drone.height - ConstVal::DroneHeight) * 0.15f);
    zoom *= zoom;
    drone.camera.getComponent<xy::Camera>().setZoom(zoom + 1.f);
    drone.velocity *= 0.9f;

    //update side view
    xy::Command cmd;
    cmd.targetFlags = CommandID::PlayerSide;
    cmd.action = [drone, sidePos](xy::Entity e, float)
    {
        auto& tx = e.getComponent<xy::Transform>();
        auto pos = tx.getPosition();
        pos.x = sidePos;
        pos.y = drone.height;
        tx.setPosition(pos);
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

    if (drone.height > ConstVal::MaxDroneHeight)
    {
        //hit the ground!
        drone.lives--;
        drone.state = Drone::State::Dead;
        drone.health = 1.f; //prevents immediately dying again when respawning with no health

        auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
        msg->type = DroneEvent::Died;
        msg->position = tx.getPosition();
        msg->lives = drone.lives;

        cmd.targetFlags = CommandID::LifeMeter;
        cmd.action = [drone](xy::Entity e, float)
        {
            e.getComponent<ItemBar>().itemCount = drone.lives;
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);


        //this is handled by GameState::showCrashMessage();
        //if (drone.lives == 0)
        //{
        //    //end the game
        //    auto* msg2 = postMessage<GameEvent>(MessageID::GameMessage);
        //    msg2->type = GameEvent::StateChange;
        //    msg2->reason = GameEvent::NoLivesLeft;
        //}
    }
}

void DroneSystem::spawnBomb(sf::Vector2f position, sf::Vector2f velocity)
{
    sf::Color orange(255, 0, 128); //that's... not orange

    //side bomb
    auto entity = getScene()->createEntity();
    entity.addComponent<xy::Transform>().setPosition((position.y) / 2.f + ConstVal::BackgroundPosition.x, ConstVal::DroneHeight);
    entity.addComponent<xy::Drawable>();
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.resize(4);
    verts[0] = { sf::Vector2f(-2.f, -8.f), orange };
    verts[1] = { sf::Vector2f(2.f, -8.f), orange };
    verts[2] = { sf::Vector2f(2.f, 8.f), orange };
    verts[3] = { sf::Vector2f(-2.f, 8.f), orange };
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.addComponent<Bomb>().position = position;
    entity.getComponent<Bomb>().velocity = velocity;

    //top view bomb
    entity = getScene()->createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::AmmoTop];
    auto bounds = m_sprites[SpriteID::AmmoTop].getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<Bomb>().position = position;
    entity.getComponent<Bomb>().velocity = velocity;
    entity.getComponent<Bomb>().height = ConstVal::DroneHeight;
    entity.getComponent<Bomb>().type = Bomb::Top;

    auto* msg = postMessage<BombEvent>(MessageID::BombMessage);
    msg->type = BombEvent::Dropped;
    msg->position = position;
}

void DroneSystem::updateAmmoBar(Drone drone)
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::AmmoMeter;
    cmd.action = [drone](xy::Entity e, float)
    {
        e.getComponent<ItemBar>().itemCount = drone.ammo;
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void DroneSystem::updateBatteryBar(Drone drone)
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::BatteryMeter;
    cmd.action = [drone](xy::Entity e, float)
    {
        sf::Color c;
        if (drone.battery < 33)
        {
            c = sf::Color::Red;
        }
        else if (drone.battery < 66)
        {
            c = sf::Color::Yellow;
        }
        else
        {
            c = sf::Color::Green;
        }

        auto& sprite = e.getComponent<xy::Sprite>();
        sprite.setColour(c);
        auto bounds = e.getComponent<sf::FloatRect>();
        bounds.width *= (drone.battery / 100.f);
        sprite.setTextureRect(bounds);
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
}