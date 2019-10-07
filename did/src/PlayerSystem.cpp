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

#include "PlayerSystem.hpp"
#include "InputBinding.hpp"
#include "PacketTypes.hpp"
#include "AnimationSystem.hpp"
#include "CollisionBounds.hpp"
#include "CollisionSystem.hpp"
#include "MessageIDs.hpp"
#include "BoatSystem.hpp"
#include "CarriableSystem.hpp"
#include "SkullShieldSystem.hpp"
#include "QuadTreeFilter.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

namespace
{
    const float PlayerSpeed = 80.f;
    const float RespawnTime = 3.f;

    const float HurtRadius = 48.f * 48.f;

    const float CurseTime = 10.f;
}

PlayerSystem::PlayerSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(PlayerSystem))
{
    requireComponent<Player>();
    requireComponent<xy::Transform>();
    requireComponent<AnimationModifier>();
}

//public
void PlayerSystem::process(float dt)
{
    auto& entities = getEntities();

    for (auto entity : entities)
    {
        auto& player = entity.getComponent<Player>();
        auto& tx = entity.getComponent<xy::Transform>();
        auto& inventory = entity.getComponent<Inventory>();
        auto& input = entity.getComponent<InputComponent>();

        //kludgy... update the player's sync flags with the carrier's
        //remember to update this if adding new player sync values
        player.sync.flags = ((player.sync.flags & 0x7) | entity.getComponent<Carrier>().carryFlags);

        //count down any curse on the player...
        if (player.sync.flags & Player::Cursed)
        {
            player.curseTimer -= dt;
            if (player.curseTimer < 0)
            {
                player.sync.flags &= ~Player::Cursed;

                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->entity = entity;
                msg->action = PlayerEvent::Cursed;
                msg->data = 0;
            }
        }

        //current input actually points to next empty slot.
        std::size_t idx = (input.currentInput + input.history.size() - 1) % input.history.size();

        if (player.sync.state == Player::Alive
            && inventory.health == 0) //TODO on the client this only wants to happen when the server says so, else it happens twice, once locally
        {
            player.sync.state = Player::Dead;
            player.respawn = RespawnTime;
            player.deathPosition = tx.getPosition();
            player.sync.flags &= ~Player::Cursed;
            player.curseTimer = 0.f;

            //need to drop anything carried
            if (entity.getComponent<Carrier>().carryFlags /*& (Carrier::Torch | Carrier::Treasure)*/)
            {
                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->entity = entity;
                msg->action = PlayerEvent::DroppedCarrying;
            }

            //broadcast
            auto* deadMsg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
            deadMsg->entity = entity;
            deadMsg->action = PlayerEvent::Died;
            deadMsg->data = inventory.lastDamager;
            deadMsg->position = tx.getPosition();

            //std::cout << entity.getIndex() << " player died\n";
        }
        else if(player.sync.state == Player::Dead)
        {
            //count down to respawn
            player.respawn -= dt;
            if (player.respawn < 0.f)
            {
                player.sync.state = Player::Alive;
                //tx.setPosition(player.spawnPosition); //This is handled by a server message
                inventory.health = -1; //prevent the player from dying twice due to still being 0 next loop
                
                //raise spawn message
                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->entity = entity;
                msg->action = PlayerEvent::Respawned;
                msg->position = player.spawnPosition;
            }
        }
        
        //used to calculate animation direction
        sf::Vector2f direction;
        while (input.lastUpdatedInput != idx)
        {
            auto delta = getDelta(input.history, input.lastUpdatedInput);
            auto currentMask = input.history[input.lastUpdatedInput].input;// .mask;

            direction = processInput(currentMask, delta, entity);
            input.lastUpdatedInput = (input.lastUpdatedInput + 1) % input.history.size();

            //getScene()->getSystem<CollisionSystem>().queryState(entity);

            //collision
            //updateCollision(entity, delta);
        }
        updateCollision(entity, dt);

        //set animation state
        updateAnimation(direction, entity);

        //update cooldown
        if (player.coolDown > 0)
        {
            player.coolDown -= dt;
            if (player.coolDown < 0)
            {
                player.sync.flags |= Player::CanDoAction;
            }
        }
    }
}

void PlayerSystem::reconcile(const ClientState& state, xy::Entity entity)
{
    auto& player = entity.getComponent<Player>();
    //urg this is a horrible kludge to prevent the server overriding the
    //animation state when the client is digging.
    auto flags = state.sync.flags | (player.sync.flags & Player::CanDoAction);
    player.sync = state.sync;
    player.sync.flags = flags;

    entity.getComponent<Carrier>().carryFlags = (player.sync.flags & ~0x7);

    //we don't actually sync the player curse time
    //but we do sync the state - so set the curse time locally
    //so that the local system doesn't try to reset the state
    player.curseTimer = CurseTime;

    //the same is true for the respawn timer
    player.respawn = RespawnTime;

    //letting the health drop to zero locally before re-syncing
    //causes the 'died' event to be raised twice, once locally
    //and once after reconciling. This fudges around that...
    if (player.sync.state == Player::Dead)
    {
        entity.getComponent<Inventory>().health = 0;
    }
    else
    {
        entity.getComponent<Inventory>().health = 1;
    }

    auto& tx = entity.getComponent<xy::Transform>();
    tx.setPosition(state.position);

    //find the oldest timestamp not used by server
    auto& input = entity.getComponent<InputComponent>();
    auto ip = std::find_if(input.history.rbegin(), input.history.rend(),
        [&state](const HistoryState& hs)
    {
        return (state.clientTime == hs.input.timestamp);
    });

    //and reparse inputs
    sf::Vector2f direction; //for animation
    if (ip != input.history.rend())
    {
        std::size_t idx = std::distance(input.history.begin(), ip.base()) - 1;
        auto end = (input.currentInput + input.history.size() - 1) % input.history.size();

        while (idx != end) //currentInput points to the next free slot in history
        {
            float delta = getDelta(input.history, idx);

            auto currentMask = input.history[idx].input;// .mask;
            direction = processInput(currentMask, delta, entity);

            idx = (idx + 1) % input.history.size();
        }
        
        //seems far less lumpy without resolving collision here...
        //getScene()->getSystem<CollisionSystem>().queryState(entity);
        //updateCollision(entity, /*delta*/1.f/60.f);
    }

    //update animation
    updateAnimation(direction, entity);
}

//private
sf::Vector2f PlayerSystem::parseInput(std::uint16_t mask, bool cursed)
{
    sf::Vector2f motion;
    if (mask & InputFlag::Left)
    {
        motion.x = -1.f;
    }
    if (mask & InputFlag::Right)
    {
        motion.x += 1.f;
    }
    if (mask & InputFlag::Up)
    {
        motion.y = -1.f;
    }
    if (mask & InputFlag::Down)
    {
        motion.y += 1.f;
    }
    if (xy::Util::Vector::lengthSquared(motion) > 1)
    {
        motion = xy::Util::Vector::normalise(motion);
    }

    //if cursed reverse controls
    if (cursed)
    {
        motion = -motion;
    }

    return motion;
}

float PlayerSystem::getDelta(const History& history, std::size_t idx)
{
    auto prevInput = (idx + history.size() - 1) % history.size();
    auto delta = history[idx].input.timestamp - history[prevInput].input.timestamp;

    return static_cast<float>(delta) / 1000000.f;
}

sf::Vector2f PlayerSystem::processInput(Input input, float delta, xy::Entity entity)
{
    auto& player = entity.getComponent<Player>();
    auto& tx = entity.getComponent<xy::Transform>();

    if (player.sync.state == Player::State::Alive)
    {
        auto motion = parseInput(input.mask, (player.sync.flags & Player::Cursed));
        if (xy::Util::Vector::lengthSquared(motion) > 0)
        {
            player.sync.accel = std::min(1.f, player.sync.accel + (delta * 4.f));
            tx.move((player.sync.accel * PlayerSpeed * input.acceleration) * motion * delta);
            player.previousMovement = motion;
        }
        else
        {
            player.sync.accel = 0.f;
        }


        std::uint16_t changes = input.mask ^ player.previousInputFlags;
        if (changes & InputFlag::CarryDrop)
        {
            if (input.mask & InputFlag::CarryDrop)
            {
                /*
                This merely raises messages to say what we want to do.
                It is up to the server to decide if we actually pick something
                up and raise a message telling the player it is safe to do so.
                */

                //button pressed
                if (entity.getComponent<Carrier>().carryFlags/* & (Carrier::Torch | Carrier::Treasure)*/)
                {
                    //raise message to drop what we're carrying
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->entity = entity;
                    msg->action = PlayerEvent::DroppedCarrying;
                }
                else
                {
                    //raise message to search for nearby carriables
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->entity = entity;
                    msg->action = PlayerEvent::WantsToCarry;
                }
            }
        }
        else if (changes & InputFlag::Action)
        {
            if ((input.mask & InputFlag::Action) && (player.sync.flags & Player::CanDoAction))
            {
                //drop anything we're carrying because we can't carry and dig/fight
                auto carryFlags = entity.getComponent<Carrier>().carryFlags;
                if (carryFlags)
                {
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->entity = entity;

                    if (carryFlags & (Carrier::Torch | Carrier::Treasure))
                    {
                        //raise message to drop what we're carrying
                        msg->action = PlayerEvent::DroppedCarrying;
                    }
                    else if(entity.getComponent<CollisionComponent>().water == 0)
                    {
                        //perform that items action
                        msg->action = PlayerEvent::ActivatedItem;
                    }
                }
                else
                {
                    //button pressed
                    const auto& inventory = entity.getComponent<Inventory>();
                    if (inventory.weapon != Inventory::Pistol || inventory.ammo > 0)
                    {
                        auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                        msg->entity = entity;
                        msg->action = PlayerEvent::DidAction; //activates selected weapon

                        player.sync.flags &= ~Player::CanDoAction;

                        switch (inventory.weapon)
                        {
                        default: break;
                        case Inventory::Pistol:
                            player.coolDown = Inventory::PistolCoolDown;
                            break;
                        case Inventory::Shovel:
                            player.coolDown = Inventory::ShovelCoolDown;
                            break;
                        case Inventory::Sword:
                            player.coolDown = Inventory::SwordCoolDown;
                            break;
                        }
                    }
                }
            }
        }
        else if (changes & InputFlag::WeaponNext)
        {
            if (input.mask & InputFlag::WeaponNext)
            {
                //button pressed
                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->entity = entity;
                msg->action = PlayerEvent::NextWeapon;
            }
            else
            {
                //button released
            }
        }
        else if (changes & InputFlag::WeaponPrev)
        {
            if (input.mask & InputFlag::WeaponPrev)
            {
                //button pressed
                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->entity = entity;
                msg->action = PlayerEvent::PreviousWeapon;
            }
            else
            {
                //button released
            }
        }

        player.previousInputFlags = input.mask;

        //check if shovel button is held and continue to dig on cooldown time
        if (input.mask & InputFlag::Action
            && entity.getComponent<Inventory>().weapon == Inventory::Shovel
            && (player.sync.flags & Player::CanDoAction))
        {
            auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
            msg->entity = entity;
            msg->action = PlayerEvent::DidAction; //activates selected weapon

            player.sync.flags &= ~Player::CanDoAction;
            player.coolDown = Inventory::ShovelCoolDown;
        }

        //when strafing direction isn't updated
        if (input.mask & InputFlag::Strafe)
        {
            player.sync.flags |= Player::Strafing;

            //auto pos = tx.getPosition();
            //const sf::FloatRect searchArea(pos.x - 128.f, pos.y - 128.f, 256.f, 256.f);
            //auto results = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, QuadTreeFilter::Player);
            //for (auto other : results)
            //{
            //    if (other != entity)
            //    {
            //        //we can safely overwrite 'motion' now as we've already
            //        //moved and it will be returned as a value to update the player direction
            //        motion = other.getComponent<xy::Transform>().getPosition() - pos;
            //        break;
            //    }
            //}
        }
        else
        {
            player.sync.flags &= ~Player::Strafing;
        }

        return motion;
    }

    return {};
}

void PlayerSystem::updateAnimation(sf::Vector2f direction, xy::Entity entity)
{
    auto& anim = entity.getComponent<AnimationModifier>();
    auto& player = entity.getComponent<Player>();

    if (xy::Util::Vector::lengthSquared(direction) > 0)
    {
        //player must be moving
        if ((player.sync.flags & Player::Strafing) == 0)
        {
            //set animation direction if not strafing
            if (direction.y > 0)
            {
                anim.nextAnimation = AnimationID::WalkDown;
                player.sync.direction = Player::Direction::Down;
            }
            else if (direction.y < 0)
            {
                anim.nextAnimation = AnimationID::WalkUp;
                player.sync.direction = Player::Direction::Up;
            }

            if (direction.x > 0)
            {
                anim.nextAnimation = AnimationID::WalkRight;
                player.sync.direction = Player::Direction::Right;
            }
            else if (direction.x < 0)
            {
                anim.nextAnimation = AnimationID::WalkLeft;
                player.sync.direction = Player::Direction::Left;
            }
        }
        else //TODO fix this repeating animation
        {
            //carry on facing the way we were when started to strafe
            switch (player.sync.direction)
            {
            default: break;
            case Player::Up:
                anim.nextAnimation = AnimationID::WalkUp;
                break;
            case Player::Down:
                anim.nextAnimation = AnimationID::WalkDown;
                break;
            case Player::Left:
                anim.nextAnimation = AnimationID::WalkLeft;
                break;
            case Player::Right:
                anim.nextAnimation = AnimationID::WalkRight;
                break;
            }
        }
    }
    else
    {
        switch (anim.currentAnimation)
        {
        default: break;
        case AnimationID::WalkDown:
            anim.nextAnimation = AnimationID::IdleDown;
            break;
        case AnimationID::WalkUp:
            anim.nextAnimation = AnimationID::IdleUp;
            break;
        case AnimationID::WalkLeft:
            anim.nextAnimation = AnimationID::IdleLeft;
            break;
        case AnimationID::WalkRight:
            anim.nextAnimation = AnimationID::IdleRight;
                break;
        }
    }
    entity.getComponent<Actor>().direction = player.sync.direction; //other clients need to know our direction
}

void PlayerSystem::updateCollision(xy::Entity entity, float dt)
{
    auto& collision = entity.getComponent<CollisionComponent>();
    auto& tx = entity.getComponent<xy::Transform>();

    for (auto i = 0u; i < collision.manifoldCount; ++i)
    {
        switch (collision.manifolds[i].ID)
        {
        default: break;
        case ManifoldID::Boat:
            tx.move(collision.manifolds[i].normal * collision.manifolds[i].penetration);
            {
                auto& player = entity.getComponent<Player>();
                auto& boat = collision.manifolds[i].otherEntity.getComponent<Boat>();
                if (boat.playerNumber == player.playerNumber
                    && (entity.getComponent<Carrier>().carryFlags & Carrier::Treasure))
                {
                    //raise message to say we got treasure to the boat
                    auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                    msg->action = PlayerEvent::StashedTreasure;
                    msg->entity = entity;
                    msg->data = entity.getIndex();

                    //hm dropping the treasure should really be setting this, but we
                    //need to stop this message firing twice
                    entity.getComponent<Carrier>().carryFlags &= ~Carrier::Treasure;
                    boat.treasureCount++;
                }
            }
            break;
        case ManifoldID::SpawnArea:
        {
            auto& player = entity.getComponent<Player>();
            if (collision.manifolds[i].otherEntity.getComponent<SpawnArea>().parent.getComponent<Boat>().playerNumber != player.playerNumber)
            {
                tx.move(collision.manifolds[i].normal * collision.manifolds[i].penetration);
            }
        }
            break;
        case ManifoldID::Terrain:
        //case ManifoldID::Barrel:
            tx.move(collision.manifolds[i].normal * collision.manifolds[i].penetration);
            break;
        case ManifoldID::Skeleton:
            tx.move(collision.manifolds[i].normal * collision.manifolds[i].penetration);
            {
                //TODO because of the reconciliation this gets called waaay too many times
                auto newMsg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                newMsg->action = PlayerEvent::TouchedSkelly;
                newMsg->entity = entity;
            }
            break;
        case ManifoldID::Explosion:
        {
            //do more damage the closer we are
            auto len2 = xy::Util::Vector::lengthSquared(tx.getPosition() - collision.manifolds[i].otherEntity.getComponent<xy::Transform>().getPosition());
            float amount = xy::Util::Math::clamp(1.f - (len2 / HurtRadius), 0.f, 1.f);
            int count = static_cast<int>(amount * 3.f) + 1;
            for (auto j = 0; j < count; ++j)
            {
                auto* newMsg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                newMsg->action = PlayerEvent::ExplosionHit;
                newMsg->entity = entity;
                newMsg->data = collision.manifolds[i].otherEntity.getComponent<std::int32_t>();
            }
        }
            break;
        case ManifoldID::SkullShield:
        {
            auto& player = entity.getComponent<Player>();
            if ((player.sync.flags & Player::Cursed) == 0
                && collision.manifolds[i].otherEntity.getComponent<SkullShield>().owner != entity.getComponent<Actor>().id)
            {
                player.sync.flags |= Player::Cursed;
                player.curseTimer = CurseTime;

                auto* msg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
                msg->entity = entity;
                msg->action = PlayerEvent::Cursed;
                msg->data = 1;
            }
        }
            break;
        }
    }

    if (collision.water >= 1) //start drowning
    {
        auto& player = entity.getComponent<Player>();
        player.drownTime -= dt;

        if (player.drownTime < 0)
        {
            auto newMsg = postMessage<PlayerEvent>(MessageID::PlayerMessage);
            newMsg->action = PlayerEvent::Drowned;
            newMsg->entity = entity;

            player.drownTime = 0.6f;
        }
    }
}