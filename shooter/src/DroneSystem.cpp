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
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

namespace
{
    const float Acceleration = 60.f;
    const float MaxVelocity = 800.f;
    const float MaxVelocitySqr = MaxVelocity * MaxVelocity;
}

DroneSystem::DroneSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(DroneSystem))
{
    requireComponent<xy::Transform>();
    requireComponent<Drone>();
}

//public
void DroneSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
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
        auto& tx = entity.getComponent<xy::Transform>();
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
        if (drone.inputFlags & Drone::Fire)
        {
            spawnBomb(pos, drone.velocity);
        }
        drone.inputFlags = 0;
    }
}

//private
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