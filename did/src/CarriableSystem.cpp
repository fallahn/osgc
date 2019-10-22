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

#include "CarriableSystem.hpp"
#include "MessageIDs.hpp"
#include "PlayerSystem.hpp"
#include "CollisionBounds.hpp"
#include "ServerSharedStateData.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"
#include "Server.hpp"
#include "AnimationSystem.hpp"
#include "QuadTreeFilter.hpp"
#include "Operators.hpp"
#include "GlobalConsts.hpp"
#include "BoatSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

sf::Vector2f operator * (sf::Vector2f l, sf::Vector2f r)
{
    return { l.x * r.x, l.y * r.y };
}

namespace
{
    const float GrabOffset = 6.f;
}

CarriableSystem::CarriableSystem(xy::MessageBus& mb, Server::SharedStateData& sd)
    : xy::System    (mb, typeid(CarriableSystem)),
    m_sharedData    (sd),
    m_stashedIndex  (0)
{
    requireComponent<Carriable>();
    requireComponent<CollisionComponent>();
    requireComponent<xy::Transform>();
}

//public
void CarriableSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        switch (data.action)
        {
        default: break;
        case PlayerEvent::WantsToCarry:
            tryGrab(data.entity);
            break;
        case PlayerEvent::DroppedCarrying:
            tryDrop(data.entity);
            break;
        case PlayerEvent::StashedTreasure:
            tryDrop(data.entity, true);
            break;
        case PlayerEvent::ActivatedItem:
            tryDrop(data.entity, true);
            break;
        }
    }
}

void CarriableSystem::process(float)
{
    //find carried entities and update offset based on parent direction
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& carriable = entity.getComponent<Carriable>();
        if (carriable.carried)
        {
            auto direction = carriable.parentEntity.getComponent<Actor>().direction;
            auto& anim = entity.getComponent<AnimationModifier>();
            sf::Vector2f position;
            switch (direction)
            {
            case Player::Left:
                position.x = -Global::CarryOffset;
                position.y -= 0.5f;
                anim.nextAnimation = AnimationID::WalkLeft;
                break;
            case Player::Right:
                position.x = Global::CarryOffset;
                position.y += 0.5f;
                anim.nextAnimation = AnimationID::WalkRight;
                break;
            case Player::Up:
                position.y = -Global::CarryOffset;
                anim.nextAnimation = AnimationID::WalkUp;
                break;
            case Player::Down:
                position.y = Global::CarryOffset;
                anim.nextAnimation = AnimationID::WalkDown;
                break;
            }
            position *= carriable.offsetMultiplier;
            entity.getComponent<xy::Transform>().setPosition(position + carriable.parentEntity.getComponent<xy::Transform>().getPosition());
        }

        //makes sure we're sending no more than 4 sets of updates for carried items
        entity.getComponent<Actor>().nonStatic = carriable.carried;
    }
}

void CarriableSystem::onEntityAdded(xy::Entity entity)
{
    entity.getComponent<Carriable>().spawnPosition = entity.getComponent<xy::Transform>().getPosition();
}

//private
void CarriableSystem::tryGrab(xy::Entity entity)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto pos = tx.getWorldPosition();
    auto bounds = entity.getComponent<CollisionComponent>().bounds * 6.f;
    bounds.left += pos.x;
    bounds.top += pos.y;

    auto entities = getScene()->getSystem<xy::DynamicTreeSystem>().query(bounds, QuadTreeFilter::Carriable);
    //check our entities to see if they are in player range
    if (entities.size() > 9)
    {
        //TODO we probably don't need to care about this any more
        xy::Logger::log("Many carriables found!");
    }

    sf::Vector2f grabOffset;
    sf::Vector2f carriablePosition;
    
    const auto& actor = entity.getComponent<Actor>();
    switch (actor.direction)
    {
    case Player::Up:
        grabOffset.y = -GrabOffset;
        carriablePosition.y = -Global::CarryOffset;
        break;
    case Player::Down:
        grabOffset.y = GrabOffset;
        carriablePosition.y = Global::CarryOffset;
        break;
    case Player::Left:
        grabOffset.x = -GrabOffset;
        carriablePosition.x = -Global::CarryOffset;
        break;
    case Player::Right:
        grabOffset.x = GrabOffset;
        carriablePosition.x = Global::CarryOffset;
        break;
    }

    auto grabPosition = tx.getPosition() + grabOffset;
    //try with a bigger box than a point
    sf::FloatRect grabBox(grabPosition.x - 5.f, grabPosition.y - 5.f, 10.f, 10.f);

    for (auto ent : entities)
    {
        auto& entTx = ent.getComponent<xy::Transform>();

        if (ent.getComponent<Actor>().id == Actor::ID::Boat)
        {
            //LOG("Grabbing in boat area", xy::Logger::Type::Info);
            if(actor.id >= Actor::PlayerOne && actor.id <= Actor::PlayerFour)
            {
                auto& boat = ent.getComponent<Boat>();
                const auto& player = entity.getComponent<Player>();
                auto& carrier = entity.getComponent<Carrier>();

                auto boatArea = entTx.getTransform().transformRect(ent.getComponent<CollisionComponent>().bounds * 1.05f);

                if (player.playerNumber != boat.playerNumber &&
                    boat.treasureCount > 0 &&
                    boatArea.intersects(grabBox))
                {
                    //sneaky stealage
                    XY_ASSERT(m_stashedIndex > 0, "Incorrect stashed treasure count");
                    auto treasureEnt = m_stashedTreasures[--m_stashedIndex];
                    auto& carriable = treasureEnt.getComponent<Carriable>();

                    carrier.carryFlags |= carriable.type;
                    carrier.carriedEntity = treasureEnt;

                    carriablePosition *= carriable.offsetMultiplier;
                    carriablePosition += tx.getPosition();
                    //entTx.setPosition(carriablePosition); //don't do this it moves the boat you wally.

                    carriable.carried = true;
                    carriable.parentEntity = entity;
                    carriable.stashed = false;

                    boat.treasureCount--;

                    //raise message so server can increase available treasure count and update boat animations
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->entity = entity;
                    msg->action = PlayerEvent::StoleTreasure;
                    msg->data = boat.playerNumber; //so we know who was stolen from
                    msg->position = carriablePosition;

                    //tell clients
                    CarriableState cs;
                    cs.action = CarriableState::PickedUp;
                    cs.carriableID = treasureEnt.getIndex();
                    cs.parentID = entity.getIndex();
                    cs.position = carriablePosition;
                    m_sharedData.gameServer->broadcastData(PacketID::CarriableUpdate, cs, xy::NetFlag::Reliable, Global::ReliableChannel);
                    break;
                }
            }
        }
        else
        {
            auto& carriable = ent.getComponent<Carriable>();
            if (!carriable.carried &&
                entTx.getTransform().transformRect(ent.getComponent<CollisionComponent>().bounds * 0.5f).intersects(grabBox))
            {
                auto& carrier = entity.getComponent<Carrier>();
                carrier.carryFlags |= carriable.type;
                carrier.carriedEntity = ent;

                carriablePosition *= carriable.offsetMultiplier;
                carriablePosition += tx.getPosition();
                entTx.setPosition(carriablePosition);

                carriable.carried = true;
                carriable.parentEntity = entity;

                //tell clients
                CarriableState cs;
                cs.action = CarriableState::PickedUp;
                cs.carriableID = ent.getIndex();
                cs.parentID = entity.getIndex();
                cs.position = carriablePosition;
                m_sharedData.gameServer->broadcastData(PacketID::CarriableUpdate, cs, xy::NetFlag::Reliable, Global::ReliableChannel);
                break;
            }
        }
    }
}

void CarriableSystem::tryDrop(xy::Entity entity, bool destroyItem)
{
    auto& carrier = entity.getComponent<Carrier>();
    auto ent = carrier.carriedEntity;

    auto& carriable = ent.getComponent<Carriable>();
    const auto& collision = ent.getComponent<CollisionComponent>();
    //do this first so any action callbacks can read carriable data before it's reset
    if (destroyItem)
    {
        if (ent.getComponent<Actor>().id == Actor::Treasure)
        {
            //stash this away so someone might steal it later...
            XY_ASSERT(m_stashedIndex < m_stashedTreasures.size(), "Stashed treasure count is incorrect");
            carriable.stashed = true;
            ent.getComponent<xy::Transform>().setPosition(-5000.f, -5000.f); //just hide somewhere
            m_stashedTreasures[m_stashedIndex++] = ent;
        }
        else if(collision.water == 0)
        {
            //was an activated item (presumably) so activate and destroy it
            carriable.action(ent, entity.getComponent<Actor>().id);
            getScene()->destroyEntity(ent);
        }
    }

    carriable.carried = false;
    carriable.parentEntity = {};

    carrier.carryFlags &= ~carriable.type;
    carrier.carriedEntity = {};

    auto& anim = ent.getComponent<AnimationModifier>();
    switch (anim.currentAnimation)
    {
    default: break;
    case AnimationID::WalkDown:
        anim.nextAnimation = AnimationID::IdleDown;
        break;
    case AnimationID::WalkLeft:
        anim.nextAnimation = AnimationID::IdleLeft;
        break;
    case AnimationID::WalkRight:
        anim.nextAnimation = AnimationID::IdleRight;
        break;
    case AnimationID::WalkUp:
        anim.nextAnimation = AnimationID::IdleUp;
        break;
    }

    //tell clients
    CarriableState cs;
    cs.action = CarriableState::Dropped;
    cs.carriableID = ent.getIndex();
    cs.parentID = entity.getIndex();
    cs.position = ent.getComponent<xy::Transform>().getPosition();
    m_sharedData.gameServer->broadcastData(PacketID::CarriableUpdate, cs, xy::NetFlag::Reliable, Global::ReliableChannel);

    //check if was dropped in water
    if (collision.water > 0 && !carriable.stashed)
    {
        //raise message so client can render sfx
        cs.action = CarriableState::InWater;
        cs.carriableID = ent.getIndex();
        cs.parentID = 0;
        cs.position = ent.getComponent<xy::Transform>().getPosition();
        m_sharedData.gameServer->broadcastData(PacketID::CarriableUpdate, cs, xy::NetFlag::Reliable, Global::ReliableChannel);

        ent.getComponent<xy::Transform>().setPosition(carriable.spawnPosition);
    }
}