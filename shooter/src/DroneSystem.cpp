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

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Wavetable.hpp>

namespace
{
    const float Acceleration = 60.f;
    const float MaxVelocity = 800.f;
    const float MaxVelocitySqr = MaxVelocity * MaxVelocity;

    const float MaxDroneHeight = 300.f; //crash after this
    const float DroneDipHeight = 290.f; //dips this low for pickups
    const float Gravity = 9.9f;
}

DroneSystem::DroneSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(DroneSystem))
{
    requireComponent<xy::Transform>();
    requireComponent<Drone>();

    m_wavetable = xy::Util::Wavetable::sine(0.25f, (DroneDipHeight - ConstVal::DroneHeight));

}

//public
void DroneSystem::handleMessage(const xy::Message& /*msg*/)
{
    //if (msg.id == MessageID::DroneMessage)
    //{
    //    const auto& data = msg.getData<DroneEvent>();
    //    if (data.type == DroneEvent::Spawned)
    //    {

    //    }
    //}
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
        case Drone::State::Dead: break;
        }
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
    pos.x = xy::Util::Math::clamp(pos.x, ConstVal::MapArea.left, ConstVal::MapArea.width);
    pos.y = xy::Util::Math::clamp(pos.y, ConstVal::MapArea.top, ConstVal::MapArea.height);
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
    if (drone.inputFlags & Drone::Fire
        && drone.ammo > 0)
    {
        drone.ammo--;
        spawnBomb(pos, drone.velocity);
    }

    if (drone.inputFlags & Drone::Pickup)
    {
        drone.state = Drone::State::PickingUp;
        entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
    }

    drone.inputFlags = 0;

    //slowly drain battery - TODO also in pick up state?
    drone.battery = std::max(0.f, drone.battery - (dt * 3.33f)); //30 seconds(ish)
    //send update to battery indicator
    cmd.targetFlags = CommandID::BatteryMeter;
    cmd.action = [drone](xy::Entity e, float)
    {
        //TODO bar is 38px x 9 px, make this const val somewhere
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

        auto& drawable = e.getComponent<xy::Drawable>();
        auto& verts = drawable.getVertices();
        verts[0].color = c;
        verts[1] = { sf::Vector2f(drone.battery * 0.38f, 0.f), c };
        verts[2] = { sf::Vector2f(drone.battery * 0.38f, 9.f), c };
        verts[3] = { sf::Vector2f(0.f, 9.f), c };
        drawable.updateLocalBounds();
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

    if (drone.battery == 0)
    {
        drone.state = Drone::State::Dying;
        entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
    }
}

void DroneSystem::processPickingUp(xy::Entity entity, float dt)
{
    auto& drone = entity.getComponent<Drone>();
    if (drone.wavetableIndex < m_wavetable.size() / 2)
    {
        drone.height = m_wavetable[drone.wavetableIndex++] + ConstVal::DroneHeight;

        //update top view
        auto& tx = entity.getComponent<xy::Transform>();
        tx.move(drone.velocity * dt);
        auto sidePos = ConstVal::BackgroundPosition.x + (tx.getPosition().y / 2.f);
        drone.camera.getComponent<xy::Camera>().setZoom(((drone.height - ConstVal::DroneHeight) * 0.5f) + 1.f);
        drone.velocity *= 0.9f;

        //update side view
        xy::Command cmd;
        cmd.targetFlags = CommandID::PlayerSide;
        cmd.action = [drone, sidePos](xy::Entity e, float dt)
        {
            auto& tx = e.getComponent<xy::Transform>();
            auto pos = tx.getPosition();
            pos.x = sidePos;
            pos.y = drone.height;
            tx.setPosition(pos);
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

        //TODO check collision with collectible

        //TODO check collision with buildings/trees
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
    drone.camera.getComponent<xy::Camera>().setZoom(((drone.height - ConstVal::DroneHeight) * 0.5f) + 1.f);
    drone.velocity *= 0.9f;

    //update side view
    xy::Command cmd;
    cmd.targetFlags = CommandID::PlayerSide;
    cmd.action = [drone, sidePos](xy::Entity e, float dt)
    {
        auto& tx = e.getComponent<xy::Transform>();
        auto pos = tx.getPosition();
        pos.x = sidePos;
        pos.y = drone.height;
        tx.setPosition(pos);
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

    if (drone.height > MaxDroneHeight)
    {
        //hit the ground!
        drone.lives--;
        drone.state = Drone::State::Dead;

        auto* msg = postMessage<DroneEvent>(MessageID::DroneMessage);
        msg->type = DroneEvent::Died;
        msg->position = tx.getPosition();
        msg->lives = drone.lives;
    }
}

void DroneSystem::spawnBomb(sf::Vector2f position, sf::Vector2f velocity)
{
    sf::Color orange(255, 0, 128);

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
    entity.addComponent<xy::Drawable>();
    auto& verts2 = entity.getComponent<xy::Drawable>().getVertices();
    verts2.resize(4);
    verts2[0] = { sf::Vector2f(-16.f, -16.f), orange };
    verts2[1] = { sf::Vector2f(16.f, -16.f), orange };
    verts2[2] = { sf::Vector2f(16.f, 16.f), orange };
    verts2[3] = { sf::Vector2f(-16.f, 16.f), orange };
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.addComponent<Bomb>().position = position;
    entity.getComponent<Bomb>().velocity = velocity;
    entity.getComponent<Bomb>().height = ConstVal::DroneHeight;
    entity.getComponent<Bomb>().type = Bomb::Top;

    auto* msg = postMessage<BombEvent>(MessageID::BombMessage);
    msg->type = BombEvent::Dropped;
    msg->position = position;
}