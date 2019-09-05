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

#include "ClientWeaponSystem.hpp"
#include "PlayerSystem.hpp"
#include "InventorySystem.hpp"
#include "MessageIDs.hpp"
#include "Actor.hpp"
#include "AnimationSystem.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/core/App.hpp>


ClientWeaponSystem::ClientWeaponSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ClientWeaponSystem)),
    m_queueSize(0)
{
    requireComponent<ClientWeapon>();
    requireComponent<xy::Transform>();
    //requireComponent<Actor>();
}

//public
void ClientWeaponSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        auto& data = msg.getData<PlayerEvent>();
        if (data.action == PlayerEvent::DidAction)
        {
            m_entityQueue[m_queueSize++] = data.entity.getIndex();
        }
    }
}

void ClientWeaponSystem::process(float)
{
    auto entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto& weapon = entity.getComponent<ClientWeapon>();

        auto parentEnt = entity.getComponent<ClientWeapon>().parent;
        const auto& actor = parentEnt.getComponent<Actor>();
        auto direction = actor.direction;

        //override with playing animation so directions better match
        auto anim = parentEnt.getComponent<AnimationModifier>().currentAnimation;// .nextAnimation;
        bool walking = false;
        switch (anim)
        {
        case AnimationID::WalkUp:
            walking = true;
        case AnimationID::IdleUp:
            direction = Player::Up;
            break;
        case AnimationID::WalkDown:
            walking = true;
        case AnimationID::IdleDown:
        case AnimationID::Die:
        case AnimationID::Spawn:
        default:
            direction = Player::Down;
            break;
        case AnimationID::WalkLeft:
            walking = true;
        case AnimationID::IdleLeft:
            direction = Player::Left;
            break;
        case AnimationID::WalkRight:
            walking = true;
        case AnimationID::IdleRight:
            direction = Player::Right;
            break;
        }
        
        //hide the weapon when carrying something
        if (actor.carryingItem)
        {
            switch (direction)
            {
            case Player::Up:
                tx.setScale(-1.f, 1.f);
                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::NoneFront]);
                entity.getComponent<xy::Drawable>().setDepth(parentEnt.getComponent<xy::Drawable>().getDepth() - 1);
                break;
            case Player::Down:
                tx.setScale(1.f, 1.f);
                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::NoneFront]);
                entity.getComponent<xy::Drawable>().setDepth(parentEnt.getComponent<xy::Drawable>().getDepth() + 1);
                break;
            case Player::Left:
                tx.setScale(1.f, 1.f);
                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::NoneSide]);
                entity.getComponent<xy::Drawable>().setDepth(parentEnt.getComponent<xy::Drawable>().getDepth() + 1);
                break;
            case Player::Right:
                tx.setScale(-1.f, 1.f);
                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::NoneSide]);
                entity.getComponent<xy::Drawable>().setDepth(parentEnt.getComponent<xy::Drawable>().getDepth() - 1);
                break;
            }
            continue;
        }

           
        //set animation first so if we also changed direction
        //then we have the correct animation.
        if (weapon.nextWeapon != weapon.prevWeapon)
        {
            //raise a message to say weapon switched
            auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
            msg->type = ActorEvent::SwitchedWeapon;
            msg->position = parentEnt.getComponent<xy::Transform>().getPosition();
            msg->id = weapon.nextWeapon;

            //swap to next weapon
            //if (direction != Player::Up)
            {
                switch (weapon.nextWeapon)
                {
                default:break;
                case Inventory::Pistol:
                    switch (direction)
                    {
                    default:
                        break;
                    case Player::Up:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleBack]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Down:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleFront]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Left:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleSide]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Right:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleSide]);
                        tx.setScale(-1.f, 1.f);
                        break;
                    }
                    break;
                case Inventory::Shovel:
                    switch (direction)
                    {
                    default:
                        break;
                    case Player::Up:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleBack]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Down:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleFront]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Left:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleSide]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Right:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleSide]);
                        tx.setScale(-1.f, 1.f);
                        break;
                    }
                    break;
                case Inventory::Sword:
                    switch (direction)
                    {
                    default:
                        break;
                    case Player::Up:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleBack]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Down:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleFront]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Left:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleSide]);
                        tx.setScale(1.f, 1.f);
                        break;
                    case Player::Right:
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleSide]);
                        tx.setScale(-1.f, 1.f);
                        break;
                    }
                    break;
                }
            }
            weapon.prevWeapon = weapon.nextWeapon;
        }

        if ((direction != weapon.prevDirection)
            || (walking != weapon.prevWalking))
        {
            switch (direction)
            {
            default:break;
            case Player::Up:
                tx.setScale(1.f, 1.f);
                switch (weapon.nextWeapon)
                {
                default:break;
                case Inventory::Pistol:
                    walking ? 
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolWalkBack])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleBack]);
                    break;
                case Inventory::Sword:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordWalkBack])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleBack]);
                    break;
                case Inventory::Shovel:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelWalkBack])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleBack]);
                    break;
                }
                break;
            case Player::Down:
                tx.setScale(1.f, 1.f);
                switch (weapon.nextWeapon)
                {
                default:break;
                case Inventory::Pistol:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolWalkFront])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleFront]);
                    break;
                case Inventory::Sword:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordWalkFront])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleFront]);
                    break;
                case Inventory::Shovel:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelWalkFront])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleFront]);
                    break;
                }
                break;
            case Player::Left:
                tx.setScale(1.f, 1.f);
                switch (weapon.nextWeapon)
                {
                default:break;
                case Inventory::Pistol:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolWalkSide])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleSide]);
                    break;
                case Inventory::Sword:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordWalkSide])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleSide]);
                    break;
                case Inventory::Shovel:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelWalkSide])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleSide]);
                    break;
                }
                break;
            case Player::Right:
                tx.setScale(-1.f, 1.f);
                switch (weapon.nextWeapon)
                {
                default:break;
                case Inventory::Pistol:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolWalkSide])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolIdleSide]);
                    break;
                case Inventory::Sword:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordWalkSide])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordIdleSide]);
                    break;
                case Inventory::Shovel:
                    walking ?
                        entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelWalkSide])
                        :
                    entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelIdleSide]);
                    break;
                }
                break;
            }
        }
        weapon.prevDirection = direction;
        weapon.prevWalking = walking;


        //look to see if we're in the queue for firing an animation
        if (m_queueSize > 0)
        {
            XY_ASSERT(m_queueSize < m_entityQueue.size(), "Queue size is " + std::to_string(m_queueSize));
            for (auto i = 0u; i < m_queueSize; ++i)
            {
                if (m_entityQueue[i] == weapon.parent.getIndex())
                {
                    m_queueSize--;
                    m_entityQueue[i] = m_entityQueue[m_queueSize];

                    auto* msg = postMessage<AnimationEvent>(MessageID::AnimationMessage);
                    msg->entity = entity;
                    msg->type = AnimationEvent::Play;

                    //if (direction != Player::Up)
                    {
                        switch (weapon.nextWeapon)
                        {
                        default:break;
                        case Inventory::Pistol:
                            switch (direction)
                            {
                            default:
                                break;
                            case Player::Up:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolBack]);
                                break;
                            case Player::Down:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolFront]);
                                break;
                            case Player::Left:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolSide]);
                                break;
                            case Player::Right:
                                tx.setScale(-1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::PistolSide]);
                                break;
                            }
                            break;
                        case Inventory::Shovel:
                            switch (direction)
                            {
                            default:
                                break;
                            case Player::Up:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelBack]);
                                break;
                            case Player::Down:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelFront]);
                                break;
                            case Player::Left:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelSide]);
                                break;
                            case Player::Right:
                                tx.setScale(-1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::ShovelSide]);
                                break;
                            }
                            break;
                        case Inventory::Sword:
                            switch (direction)
                            {
                            default:
                                break;
                            case Player::Up:
                                tx.setScale(-1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordFront]);
                                break;
                            case Player::Down:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordFront]);
                                break;
                            case Player::Left:
                                tx.setScale(1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordSide]);
                                break;
                            case Player::Right:
                                tx.setScale(-1.f, 1.f);
                                entity.getComponent<xy::SpriteAnimation>().play(m_animationIDs[WeaponAnimationID::SwordSide]);
                                break;
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }

        //make sure sprite order is correct
        switch (direction)
        {
        default:
        case Player::Up:
        case Player::Right:
            entity.getComponent<xy::Drawable>().setDepth(parentEnt.getComponent<xy::Drawable>().getDepth() - 1);
            break;
        case Player::Down:
        case Player::Left:
            entity.getComponent<xy::Drawable>().setDepth(parentEnt.getComponent<xy::Drawable>().getDepth() + 1);
            break;
        }
    }
}

void ClientWeaponSystem::setAnimations(const xy::SpriteSheet& spriteSheet)
{
    m_animationIDs[WeaponAnimationID::PistolFront] = spriteSheet.getAnimationIndex("shoot_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::PistolBack] = spriteSheet.getAnimationIndex("shoot_back", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::PistolSide] = spriteSheet.getAnimationIndex("shoot_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordFront] = spriteSheet.getAnimationIndex("slash_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordSide] = spriteSheet.getAnimationIndex("slash_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelSide] = spriteSheet.getAnimationIndex("dig_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelFront] = spriteSheet.getAnimationIndex("dig_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelBack] = spriteSheet.getAnimationIndex("dig_back", "weapon_rodney");

    m_animationIDs[WeaponAnimationID::PistolWalkFront] = spriteSheet.getAnimationIndex("pistol_walk_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::PistolWalkBack] = spriteSheet.getAnimationIndex("pistol_walk_back", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::PistolWalkSide] = spriteSheet.getAnimationIndex("pistol_walk_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordWalkFront] = spriteSheet.getAnimationIndex("sword_walk_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordWalkBack] = spriteSheet.getAnimationIndex("sword_walk_back", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordWalkSide] = spriteSheet.getAnimationIndex("sword_walk_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelWalkFront] = spriteSheet.getAnimationIndex("shovel_walk_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelWalkBack] = spriteSheet.getAnimationIndex("shovel_walk_back", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelWalkSide] = spriteSheet.getAnimationIndex("shovel_walk_side", "weapon_rodney");

    m_animationIDs[WeaponAnimationID::PistolIdleFront] = spriteSheet.getAnimationIndex("pistol_idle_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::PistolIdleBack] = spriteSheet.getAnimationIndex("pistol_idle_back", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::PistolIdleSide] = spriteSheet.getAnimationIndex("pistol_idle_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordIdleFront] = spriteSheet.getAnimationIndex("sword_idle_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordIdleBack] = spriteSheet.getAnimationIndex("sword_idle_back", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::SwordIdleSide] = spriteSheet.getAnimationIndex("sword_idle_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelIdleFront] = spriteSheet.getAnimationIndex("shovel_idle_front", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelIdleBack] = spriteSheet.getAnimationIndex("shovel_idle_back", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::ShovelIdleSide] = spriteSheet.getAnimationIndex("shovel_idle_side", "weapon_rodney");

    m_animationIDs[WeaponAnimationID::NoneSide] = spriteSheet.getAnimationIndex("none_side", "weapon_rodney");
    m_animationIDs[WeaponAnimationID::NoneFront] = spriteSheet.getAnimationIndex("none_front", "weapon_rodney");
}