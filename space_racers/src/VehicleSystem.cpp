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

#define FAST_SIN 1
#ifdef FAST_SIN
#include "FastTrig.hpp"
#endif //FAST_SIN

#include "VehicleSystem.hpp"
#include "InputBinding.hpp"
#include "ServerPackets.hpp"
#include "AsteroidSystem.hpp"
#include "WayPoint.hpp"
#include "MessageIDs.hpp"
#include "NetActor.hpp"
#include "ActorIDs.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Const.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/gui/Gui.hpp>

#include <cmath>

VehicleSystem::VehicleSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(VehicleSystem))
{
    requireComponent<Vehicle>();
    requireComponent<xy::Transform>();
    requireComponent<xy::BroadphaseComponent>();
}

//public
void VehicleSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& vehicle = entity.getComponent<Vehicle>();

        //process outstanding input (current input actually points to next empty slot)
        std::size_t idx = (vehicle.currentInput + vehicle.history.size() - 1 ) % vehicle.history.size();

        while (vehicle.lastUpdatedInput != idx)
        {
            auto delta = getDelta(vehicle.history, vehicle.lastUpdatedInput);
            
            processVehicle(entity, delta);

            //check to see if the player went AFK
            if (vehicle.history[vehicle.lastUpdatedInput].flags != 0)
            {
                vehicle.afkTimer.restart();
                vehicle.stateFlags &= ~(1 << Vehicle::AFK);
            }
            if (vehicle.afkTimer.getElapsedTime().asSeconds() > Vehicle::AfkTime
                && ((vehicle.stateFlags & (1 << Vehicle::AFK)) == 0))
            {
                vehicle.stateFlags |= (1 << Vehicle::AFK);

                auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
                msg->type = VehicleEvent::WentAfk;
                msg->entity = entity;
            }
            /*if (vehicle.afkTimer.getElapsedTime().asSeconds() > Vehicle::AfkTimeout)
            {
                std::cout << "request kick AFK player!\n";
            }*/

            vehicle.lastUpdatedInput = (vehicle.lastUpdatedInput + 1) % vehicle.history.size();
        }
    }
}

void VehicleSystem::reconcile(const ClientUpdate& update, xy::Entity entity)
{
    //sync state
    auto& vehicle = entity.getComponent<Vehicle>();
    vehicle.velocity = { update.velX, update.velY };
    vehicle.anglularVelocity = update.velRot;
    vehicle.collisionFlags = update.collisionFlags;
    vehicle.stateFlags = update.stateFlags;

    auto& tx = entity.getComponent<xy::Transform>();
    tx.setPosition(update.x, update.y);
    tx.setRotation(update.rotation);

    //redo prediction from last known server input
    auto ip = std::find_if(vehicle.history.rbegin(), vehicle.history.rend(),
        [&update](const Input & input)
        {
            return update.clientTimestamp == input.timestamp;
        });

    if (ip != vehicle.history.rend())
    {
        std::size_t idx = std::distance(vehicle.history.begin(), ip.base()) - 1;
        auto end = (vehicle.currentInput + vehicle.history.size() - 1) % vehicle.history.size();

        while (idx != end) //currentInput points to the next free slot in history
        {
            float delta = getDelta(vehicle.history, idx);

            processVehicle(entity, delta);

            idx = (idx + 1) % vehicle.history.size();
        }
    }
}

//private
void VehicleSystem::processVehicle(xy::Entity entity, float delta)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    switch (vehicle.stateFlags)
    {
    default: break;
    case (1 << Vehicle::Normal):
        processInput(entity, delta);
    //this is meant to fall through
    case (1 << Vehicle::Disabled):
        applyInput(entity, delta);
        doCollision(entity);
        break;
    case (1 << Vehicle::Falling):
        updateFalling(entity, delta);
        break;
    case (1 << Vehicle::Exploding):
        updateExploding(entity, delta);
        break;
    }
    vehicle.invincibleTime -= delta;

    //checks where we are along the track
    if (vehicle.currentWaypoint.isValid())
    {
        auto relPosition = entity.getComponent<xy::Transform>().getPosition() - vehicle.currentWaypoint.getComponent<xy::Transform>().getPosition();
        float dist = xy::Util::Vector::dot(relPosition, vehicle.currentWaypoint.getComponent<WayPoint>().nextPoint);
        vehicle.totalDistance = vehicle.waypointDistance + dist;
    }
}

void VehicleSystem::processInput(xy::Entity entity, float delta)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    auto input = vehicle.history[vehicle.lastUpdatedInput];

    //acceleration
    auto acceleration = 0.f;
    if (input.flags & InputFlag::Accelerate)
    {
        vehicle.accelerationMultiplier = std::min(1.f, vehicle.accelerationMultiplier + (delta * vehicle.settings.accelStrength));
        acceleration += vehicle.settings.acceleration * input.accelerationMultiplier * vehicle.accelerationMultiplier;
    }
    else
    {
        vehicle.accelerationMultiplier = 0.f;
    }

    if (input.flags & InputFlag::Reverse)
    {
        acceleration -= vehicle.settings.acceleration * 0.3f;
    }
    //this makes sure we turn in the correct direction when reversing
    float sign = (acceleration < 0) ? -360.f : 0.f;

    //rotation
    const auto& tx = entity.getComponent<xy::Transform>();
    auto rotation = (tx.getRotation() - sign) * xy::Util::Const::degToRad;

#ifdef FAST_SIN
    //TODO if the table is pre-baked it would also be more consistent across platforms
    vehicle.velocity += sf::Vector2f(ft::cos(rotation), ft::sin(rotation)) * acceleration;
#else
    vehicle.velocity += sf::Vector2f(std::cos(rotation), std::sin(rotation)) * acceleration;
#endif //FAST_SIN

    if (input.flags & InputFlag::Brake)
    {
        vehicle.velocity *= vehicle.settings.brakeStrength;
    }

    if (input.flags & InputFlag::Left)
    {
        vehicle.anglularVelocity -= vehicle.settings.turnSpeed * input.steeringMultiplier;
    }

    if (input.flags & InputFlag::Right)
    {
        vehicle.anglularVelocity += vehicle.settings.turnSpeed * input.steeringMultiplier;
    }
}

void VehicleSystem::applyInput(xy::Entity entity, float dt)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    auto& tx = entity.getComponent<xy::Transform>();

    float drag = vehicle.settings.drag;
    float angularDrag = vehicle.settings.angularDrag;

    //NOTE TO FUTURE SELF this could be a good way of adding 'speed boost' pads
    //if (vehicle.collisionFlags & (1 << CollisionObject::Type::Jump))
    //{
    //    drag *= 1.06f;
    //    angularDrag *= 1.09f;
    //}

    tx.move(vehicle.velocity * dt);
    vehicle.velocity *= drag;// vehicle.settings.drag;

    //rotate more slowly at lower speeds
    float rotationMultiplier = std::min(1.f, xy::Util::Vector::lengthSquared(vehicle.velocity) / (vehicle.settings.maxSpeedSqr() / 8.f));
    rotationMultiplier = 0.03f + (rotationMultiplier * 0.97f);

    tx.rotate(((xy::Util::Const::radToDeg * vehicle.anglularVelocity) * rotationMultiplier) * dt);
    vehicle.anglularVelocity *= angularDrag;// vehicle.settings.angularDrag;

    //if we switched back to normal state scale will be wrong
    tx.setScale(1.f, 1.f);
}

float VehicleSystem::getDelta(const History& history, std::size_t idx)
{
    auto prevInput = (idx + history.size() - 1) % history.size();

    //skip any inputs in the wrong order - actually enet is supposed to guarantee this for us?
    /*while (history[prevInput].timestamp > history[idx].timestamp)
    {
        prevInput = (prevInput + history.size() - 1) % history.size();
    }*/

    auto delta = history[idx].timestamp - history[prevInput].timestamp;

    return static_cast<float>(delta) / 1000000.f;
}

void VehicleSystem::doCollision(xy::Entity entity)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    vehicle.collisionFlags = 0;

    auto& tx = entity.getComponent<xy::Transform>();

    //broad phase
    auto queryArea = tx.getTransform().transformRect(entity.getComponent<xy::BroadphaseComponent>().getArea());
    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(queryArea, CollisionFlags::All);

    for (auto other : nearby)
    {
        if (other != entity)
        {
            auto otherBounds = other.getComponent<xy::Transform>().getTransform().transformRect(other.getComponent<xy::BroadphaseComponent>().getArea());
            if (otherBounds.intersects(queryArea))
            {
                //we're only doing this on the current vehicle, so let's narrow phase right away!
                auto type = other.getComponent<CollisionObject>().type;
                if(type == CollisionObject::Space
                    && contains(tx.getPosition(), other))
                {
                    if (vehicle.stateFlags != (1 << Vehicle::Disabled)
                        && vehicle.type != Vehicle::Type::Ship)
                    {
                        vehicle.collisionFlags |= (1 << type);
                        vehicle.stateFlags = (1 << Vehicle::Falling);
                        vehicle.respawnClock.restart();

                        auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
                        msg->type = VehicleEvent::Fell;
                        msg->entity = entity;
                    }
                }
                else if (type == CollisionObject::Waypoint)
                {
                    vehicle.collisionFlags |= (1 << type);

                    const auto& waypoint = other.getComponent<WayPoint>();
                    if (!vehicle.currentWaypoint.isValid())
                    {
                        if (waypoint.id == 0)
                        {
                            vehicle.currentWaypoint = other;
                        }
                        else
                        {
                            explode(entity);
                        }
                    }
                    else if (waypoint.id != vehicle.currentWaypoint.getComponent<WayPoint>().id)
                    {
                        auto& vehicleWaypoint = vehicle.currentWaypoint.getComponent<WayPoint>();
                        auto expectedID = ((vehicleWaypoint.id + 1) % vehicle.waypointCount);
                        if(expectedID == waypoint.id)
                        {
                            vehicle.currentWaypoint = other;
                            vehicle.waypointDistance += waypoint.distance;
                            
                            if (waypoint.id == 0)
                            {
                                //we did a lap so raise a message
                                auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
                                msg->type = VehicleEvent::LapLine;
                                msg->entity = entity;

                                vehicle.waypointDistance = 0.f;
                            }
                        }
                        else if(waypoint.id > expectedID % vehicle.waypointCount)
                        {
                            explode(entity); //taking shortcuts or going backwards
                        }
                    }
                }

                else if (auto manifold = intersects(entity, other); manifold)
                {
                    resolveCollision(entity, other, *manifold);

                    if (vehicle.stateFlags != (1 << Vehicle::Normal))
                    {
                        //something happened so quit collision detection
                        return;
                    }
                }
            }
        }
    }
}

void VehicleSystem::resolveCollision(xy::Entity entity, xy::Entity other, Manifold manifold)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    auto& tx = entity.getComponent<xy::Transform>();

    auto separate = [&vehicle](sf::Vector2f otherVel)->float
    {
        float direction = xy::Util::Vector::dot(vehicle.velocity, otherVel);

        if (direction > 0)
        {
            //heading the same way
            vehicle.velocity -= otherVel * 0.2f;
        }
        else
        {
            //heading towards
            vehicle.velocity += otherVel;
        }
        return direction;
    };

    const auto& otherCollision = other.getComponent<CollisionObject>();
    switch (otherCollision.type)
    {
    default: break;
    case CollisionObject::Collision:
    {
        tx.move(manifold.penetration * manifold.normal);    
        vehicle.velocity = xy::Util::Vector::reflect(vehicle.velocity, manifold.normal) * 0.9f;
    }
        break;
    case CollisionObject::Jump:
        //for now we'll let the flag state do the signalling
        break;
    case CollisionObject::KillZone:
        explode(entity);
        break;
    case CollisionObject::Roid:
    {
        tx.move(manifold.penetration * manifold.normal);

        auto& roid = other.getComponent<Asteroid>();
        separate(roid.getVelocity());
               
        if (vehicle.stateFlags == (1 << Vehicle::Disabled)
            || vehicle.invincibleTime > 0)
        {
            other.getComponent<xy::Transform>().move(-manifold.penetration * manifold.normal);
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), -manifold.normal));
        }
        else if (manifold.penetration < -14)
        {
            explode(entity);
        }
    }
        break;
    case CollisionObject::Vehicle:
    {
        tx.move(manifold.penetration * manifold.normal);

        auto& otherVehicle = other.getComponent<Vehicle>();
        vehicle.velocity *= 0.5f;
        otherVehicle.velocity += vehicle.velocity;
        //TODO other vehicle should probably rotate when not hit square
        //something like (1- dot(vel, norm(otherForwardVec) * someForce)

        /*const auto& otherTx = other.getComponent<xy::Transform>();
        sf::Vector2f forwardVec = otherTx.getTransform().transformPoint({ 1.f, 0.f }) - otherTx.getPosition();
        float rotation = (1.f - xy::Util::Vector::dot(vehicle.velocity, forwardVec)) * 0.0001f;
        otherVehicle.anglularVelocity -= rotation;*/
    }
        break;
    case CollisionObject::NetActor:
    {
        //done client side to better predict collisions
        tx.move(manifold.penetration * manifold.normal);

        const auto& actor = other.getComponent<NetActor>();
        if (actor.actorID == ActorID::Roid)
        {
            separate(actor.velocity);
        }
        else
        {
            vehicle.velocity *= 0.5f;
            vehicle.velocity += actor.velocity * 0.5f;
        }
    }
        break;
    }

    //mark this type of collision active
    vehicle.collisionFlags |= (1 << otherCollision.type);
    
}

void VehicleSystem::updateFalling(xy::Entity entity, float dt)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& vehicle = entity.getComponent<Vehicle>();

    const float scaleFactor = dt * 56.f;

    tx.move(vehicle.velocity * dt);
    vehicle.velocity *= scaleFactor;
    vehicle.accelerationMultiplier = 0.f;

    const float rotation = vehicle.anglularVelocity > 0 ? 840.f : -840.f;
    tx.rotate(rotation * dt);
    tx.scale(scaleFactor, scaleFactor);

    if(vehicle.respawnClock.getElapsedTime().asSeconds() > Vehicle::RespawnDuration)
    {
        vehicle.respawnClock.restart(); //just gives a small buffer to prevent mutliple messages

        //raise a message here so that the server can be the arbiter on resetting the vehicle
        auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
        msg->type = VehicleEvent::RequestRespawn;
        msg->entity = entity;
    }
}

void VehicleSystem::updateExploding(xy::Entity entity, float dt)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    vehicle.accelerationMultiplier = 0.f;

    entity.getComponent<xy::Transform>().setScale(0.f, 0.f);

    if (vehicle.respawnClock.getElapsedTime().asSeconds() > Vehicle::RespawnDuration)
    {
        vehicle.respawnClock.restart(); //just gives a small buffer to prevent mutliple messages

        //raise a message here so that the server can be the arbiter on resetting the vehicle
        auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
        msg->type = VehicleEvent::RequestRespawn;
        msg->entity = entity;
    }
}

void VehicleSystem::explode(xy::Entity entity)
{
    entity.getComponent<xy::Transform>().setScale(0.f, 0.f);
    entity.getComponent<Vehicle>().respawnClock.restart();
    entity.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Exploding);

    auto* msg = postMessage<VehicleEvent>(MessageID::VehicleMessage);
    msg->type = VehicleEvent::Exploded;
    msg->entity = entity;
}

//we'll use this just to do some debug assertions that our vehicles are valid
void VehicleSystem::onEntityAdded(xy::Entity entity)
{
#ifdef XY_DEBUG
    const auto& vehicle = entity.getComponent<Vehicle>();
    XY_ASSERT(vehicle.waypointCount > 0, "Vehicle waypoints not set!");
#endif
}