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

#include "BotSystem.hpp"
#include "PathFinder.hpp"
#include "PlayerSystem.hpp"
#include "GlobalConsts.hpp"
#include "ServerRandom.hpp"
#include "InputBinding.hpp"
#include "CollisionBounds.hpp"
#include "SharedStateData.hpp"
#include "Packet.hpp"
#include "PacketTypes.hpp"
#include "Server.hpp"
#include "ServerRandom.hpp"
#include "QuadTreeFilter.hpp"
#include "BoatSystem.hpp"
#include "Operators.hpp"
#include "MessageIDs.hpp"
#include "WetPatchDirector.hpp"
#include "CarriableSystem.hpp"
#include "SkeletonSystem.hpp"
#include "InputParser.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Math.hpp>

#include <algorithm>

#define NO_LOG
#ifdef NO_LOG
#undef LOG
#define LOG(x,y)
#endif

/*
Behaviour:
    Searching - 
    Points are picked one at a time from a list of randomly distributed targets.
    While traversing points searches are made for diggable areas, treasure chests
    and other player. Encountering any of these switches the mode

    Fighting - 
    Engaged in combat with an identified target. This state remains until one or
    other parties have died, or moved out of range

    Capturing - currently in posession of a treasure chest and trying to return
    it to the boat. 50/50 chance of fight or flight when encountering an enemy,
    possibly higher chance of flight if near to home base already.
*/

namespace
{
    const float MinDistance = 10.f * 10.f;// (Global::TileSize/* * 4.f*/) * (Global::TileSize/* * 4.f*/);
    const float DeadZone = 2.f;
    const float TestOffset = 6.f; //distance from bot to dig

    const float BoatRadius = 80.f;
    const float BoatRadiusSqr = BoatRadius * BoatRadius;

    const float SweepTime = 1.f; //how frequently to sweep the area for interfering events
    const float SearchTime = 0.6f; //how frequently to search nearby area for dig spots

    const float MaxMoveTime = 3.f; //if we didn't find a new node in time, reset the path

    const float MaxFightDistance = 96.f * 96.f; //approx 3 tiles radius
    const float IdealSwordFightingDistance = 24.f * 24.f;
    const float MinSwordFightingDistance = 16.f * 16.f;
    const float IdealPistolFightingDistance = 96.f * 96.f;
    const float MinPistolFightingDistance = 32.f * 32.f;

    //if a target is closer than this then we don't calc a path (see sweep())
    const float MinTargetArea = ((Global::TileSize * 2.f) * (Global::TileSize * 2.f)); 

    const sf::FloatRect DaySearchArea(-160.f, -288.f, 320.f, 480.f); //a 10x15 tile square
    const sf::FloatRect NightSearchArea(-Global::LightRadius, -Global::LightRadius, Global::LightRadius * 2.f, Global::LightRadius * 2.f);

    enum class SweepResult
    {
        Fight,
        Target,
        None
    };

#ifdef XY_DEBUG
    //used for printing actors in debug mode
    static const std::vector<std::string> ActorNames =
    {
        "None", "Player One", "Player Two", "Player Three", "Player Four",
        "Skeleton", "Barrel", "Crab", "Boat", "Treasure", "Ammo", "Coin",
        "Food", "Lantern", "Treasure Spawn", "Ammo Spawn", "Coin Spawn",
        "Weapon", "Fire", "Parrot", "Explosion", "Dirt Spray", "Lightning",
        "Bees", "Skull Shield", "Skull Item", "Smoke Puff", "Flare", "Flare Item",
        "Decoy", "Decoy Item", "Magnet", "Impact"
    };
#endif
}

BotSystem::BotSystem(xy::MessageBus& mb, PathFinder& pf, std::size_t seed)
    : xy::System        (mb, typeid(BotSystem)),
    m_pathFinder        (pf),
    m_timeAccumulator   (0),
    m_rndEngine         (seed),
    m_dayPosition       (0.f)
{
    requireComponent<Player>();
    requireComponent<Bot>();

    for (auto i = 0; i < 2; i++)
    {
        auto destinationPoints = xy::Util::Random::poissonDiscDistribution(
            { 0.f, 0.f, static_cast<float>(Global::TileCountX - 12), static_cast<float>(Global::TileCountY - 12) },
            8.f, 8, m_rndEngine);

        m_destinationPoints.insert(m_destinationPoints.end(), destinationPoints.begin(), destinationPoints.end());
    }

    //remember to scale up points from grid to map size...
    for (auto& p : m_destinationPoints)
    {
        p *= Global::TileSize;
        p.x += Global::TileSize * 6.f;
        p.y += Global::TileSize * 6.f;
    }

    //erase any points which may have ended up near a boat
    const std::array<sf::Vector2f, 4u> boatPoints =
    {
        sf::Vector2f(Global::BoatXOffset, Global::BoatYOffset),
        sf::Vector2f(Global::IslandSize.x - Global::BoatXOffset, Global::BoatYOffset),
        Global::IslandSize - sf::Vector2f(Global::BoatXOffset, Global::BoatYOffset),
        sf::Vector2f(Global::BoatXOffset, Global::IslandSize.y - Global::BoatYOffset)
    };
    const sf::FloatRect boatBounds(-70.f, 40.f, 140.f, 80.f);

    const float cornerSize = 100.f;
    const std::array<sf::Vector2f, 4u> cornerPoints = 
    {
        sf::Vector2f(), sf::Vector2f(cornerSize, 0.f),
        sf::Vector2f(cornerSize, cornerSize), sf::Vector2f(0.f, cornerSize)
    };
    const sf::FloatRect cornerBounds(0.f, 0.f, cornerSize, cornerSize);

    //std::cout << m_destinationPoints.size() << "\n";
    m_destinationPoints.erase(std::remove_if(m_destinationPoints.begin(), m_destinationPoints.end(), 
        [&boatPoints, &cornerPoints, boatBounds, cornerBounds](const sf::Vector2f& point) 
    {
        for (auto i = 0u; i < 4u; ++i)
        {
            auto bbounds = boatBounds;
            bbounds.left += boatPoints[i].x;
            bbounds.top += boatPoints[i].y;

            auto cbounds = cornerBounds;
            cbounds.left += cornerPoints[i].x;
            cbounds.top += cornerPoints[i].y;

            return bbounds.contains(point) || cbounds.contains(point);
        }
        return false;
    }), m_destinationPoints.end());

    //make the points an odd number so any bots with a +2 stride will
    //get the intermediate points the second loop around
    if (m_destinationPoints.size() % 2 == 0)
    {
        m_destinationPoints.pop_back();
    }
}

//public
void BotSystem::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.action == PlayerEvent::Respawned)
        {
            if (data.entity.hasComponent<Bot>())
            {
                auto entity = data.entity;
                auto& bot = entity.getComponent<Bot>();
                if (bot.enabled)
                {
                    bot.movementTimer = 0.f;
                    bot.resetState();
                    bot.targetPoint = m_destinationPoints[Server::getRandomInt(0, m_destinationPoints.size() - 1)];
                    LOG("Reset bot " + ActorNames[entity.getComponent<Actor>().id], xy::Logger::Type::Info);
                }
            }
        }
        else if (data.action == PlayerEvent::Died)
        {
            //tell anyone fighting the dead player to stop fighting
            auto& entities = getEntities();
            for (auto entity : entities)
            {
                auto& bot = entity.getComponent<Bot>();
                if (bot.state == Bot::State::Fighting
                    && bot.targetEntity == data.entity)
                {
                    bot.targetEntity = {};
                    bot.targetType = Bot::Target::None;
                    bot.state = Bot::State::Searching;
                    bot.path.clear();
                    bot.targetPoint = {};
                    LOG(ActorNames[entity.getComponent<Actor>().id] + " gave up fighting, " + ActorNames[data.entity.getComponent<Actor>().id] + " dead", xy::Logger::Type::Info);
                }
            }
        }
        else if (data.action == PlayerEvent::BeeHit)
        {
            if (data.entity.hasComponent<Bot>())
            {
                auto ent = data.entity;
                auto& bot = ent.getComponent<Bot>();
                if (bot.enabled) 
                {
                    //check if carrying a decoy and deploy
                    if (ent.getComponent<Carrier>().carryFlags & Carrier::Decoy)
                    {
                        bot.inputMask |= InputFlag::CarryDrop;
                    }
                    else if (!bot.fleeing)
                    {
                        LOG(ActorNames[ent.getComponent<Actor>().id] + ": flee bees!", xy::Logger::Type::Info);
                        //run away
                        
                        if (ent.getComponent<Carrier>().carryFlags)
                        {
                            bot.inputMask |= InputFlag::CarryDrop;
                        }

                        bot.resetState();

                        bot.fleeing = true;
                        bot.fleeTimer = -8.f;
                        bot.sweepTimer = -14.f;

                        auto pos = ent.getComponent<xy::Transform>().getPosition();
                        if (pos.x < (Global::IslandSize.x / 2.f))
                        {
                            pos.x = 0.f;
                        }
                        else
                        {
                            pos.x = Global::IslandSize.x;
                        }
                        bot.targetPoint = pos;
                        bot.path.clear();
                        bot.path.push_back(bot.targetPoint);
                        bot.state = Bot::State::Searching;
                        //requestPath(ent);
                    }
                }
            }
        }
    }
    else if (msg.id == MessageID::MapMessage)
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::DayNightUpdate)
        {
            m_dayPosition = data.value;
        }
    }
}

void BotSystem::process(float dt)
{
    m_timeAccumulator += static_cast<std::int32_t>(dt * 1000000.f);

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& bot = entity.getComponent<Bot>();
        bot.inputMask = 0;

        if (bot.enabled)
        {
            //if for some reason we popped a state and the 
            //previous target is now a long way away, reset the state
            auto dir = bot.targetPoint - entity.getComponent<xy::Transform>().getPosition();
            if (bot.state == Bot::State::Targeting &&
                xy::Util::Vector::lengthSquared(dir) > std::pow((25.f * Global::TileSize), 2.f))
            {
                bot.resetState();
            }

            //do a sweep to see if nearby items, other players or enemies are nearby
            bot.sweepTimer += dt;
            if (bot.sweepTimer > SweepTime
                && !bot.fleeing)
            {
                bot.sweepTimer = 0.f;
                wideSweep(entity);
            }

            //check if we're done fleeing
            bot.fleeTimer += dt;
            if (bot.fleeing &&
                (bot.fleeTimer > Bot::FleeTime
                || entity.getComponent<CollisionComponent>().water > 0))
            {
                bot.fleeing = false;
                bot.resetState();
            }

            const auto inventory = entity.getComponent<Inventory>();
            //check if we have the weapon we want
            auto currWeapon = inventory.weapon;
            if (currWeapon != bot.desiredWeapon)
            {
                bot.buttonTimer -= dt;
                if (bot.buttonTimer < 0)
                {
                    bot.inputMask |= InputFlag::WeaponNext;
                    bot.buttonTimer = 0.5f;
                }
            }

            //if we have an item is it time to use it?
            if (entity.getComponent<Carrier>().carryFlags & (Carrier::Flare | Carrier::SpookySkull | Carrier::Decoy | Carrier::Mine))
            {
                bot.itemTimer -= dt;
                if (bot.itemTimer < 0)
                {
                    bot.inputMask |= InputFlag::Action;
                }
            }
            //track health
            bot.previousHealth = inventory.health;

            //update current state
            switch (bot.state)
            {
            default: break;
            case Bot::State::Searching:
                updateSearching(entity, dt);
                break;
            case Bot::State::Digging:
                updateDigging(entity, dt);
                break;
            case Bot::State::Fighting:
                updateFighting(entity, dt);
                break;
            case Bot::State::Capturing:
                updateCapturing(entity);
                break;
            case Bot::State::Targeting:
                updateTargeting(entity, dt);
                break;
            }

            //make sure we're properly completing paths
            if (bot.path.size() == 1)
            {
                const auto dist = xy::Util::Vector::lengthSquared(entity.getComponent<xy::Transform>().getPosition() - bot.path.back());
                if (dist < MinDistance)
                {
                    bot.path.clear();
                }
            }

            if (!bot.path.empty())
            {
                bot.movementTimer += dt;
                if (bot.movementTimer > MaxMoveTime)
                {
                    bot.movementTimer = 0.f;
                    bot.resetState();
                }
            }

            //apply input to player
            auto& playerIp = entity.getComponent<InputComponent>();
            playerIp.history[playerIp.currentInput].input.mask = bot.inputMask;
            playerIp.history[playerIp.currentInput].input.timestamp = m_timeAccumulator;
            playerIp.history[playerIp.currentInput].input.acceleration = bot.acceleration;
            playerIp.currentInput = (playerIp.currentInput + 1) % playerIp.history.size();
        }
    }
}

//private
bool BotSystem::isDay() const
{
    return (m_dayPosition > 0.25f && m_dayPosition < 0.5f) || (m_dayPosition > 0.75f && m_dayPosition < 1.f);
}

void BotSystem::updateSearching(xy::Entity entity, float dt)
{
    auto& bot = entity.getComponent<Bot>();
       
    if (bot.fleeing)
    {
        //path fetching is async, so we might
        //end up being supplied with a previous request
        if (bot.path.size() > 1)
        {
            bot.path.clear();
            bot.path.push_back(bot.targetPoint);
        }

        bot.pathRequested = false;
        followPath(entity);

        if (bot.path.empty())
        {
            bot.fleeing = false;
            bot.resetState();
        }
        return;
    }

    if (entity.getComponent<Carrier>().carryFlags & Carrier::Treasure)
    {
        //we got confused about whether or not we dropped treasure
        bot.targetEntity = {};
        bot.targetType = Bot::Target::None;
        bot.state = Bot::State::Capturing;
        bot.path.clear();
        bot.pathRequested = false;

        bot.targetPoint = entity.getComponent<Player>().spawnPosition;
        bot.targetPoint.y -= Global::PlayerSpawnOffset;
        requestPath(entity);

        return;
    }

    //request new path if needed
    if ((bot.path.empty() && !bot.pathRequested))
    {
        if (bot.stateStack.empty())
        {
            bot.targetPoint = m_destinationPoints[bot.pointIndex];
            bot.pointIndex = (bot.pointIndex + bot.indexStride) % m_destinationPoints.size();
            bot.desiredWeapon = entity.getComponent<Inventory>().ammo == 0 ? Inventory::Sword : Inventory::Pistol;

            requestPath(entity);
        }
        else
        {
            bot.popState();
            return;
        }
        
    }
    else if(!bot.path.empty())
    {
        bot.pathRequested = false;
        followPath(entity);

        //check for diggable holes
        bot.searchTimer += dt;
        if (bot.searchTimer > SearchTime
            && !bot.fleeing)
        {
            bot.searchTimer = 0.f;

            auto position = entity.getComponent<xy::Transform>().getPosition();
            auto bounds = isDay() ? DaySearchArea : NightSearchArea;
            bounds.left += position.x;
            bounds.top += position.y;

            const auto& tree = getScene()->getSystem<xy::DynamicTreeSystem>();
            auto patches = tree.query(bounds, QuadTreeFilter::WetPatch | QuadTreeFilter::Boat);

            for (auto found : patches)
            {
                //skip if not line of sight
                if (!m_pathFinder.rayTest(position, found.getComponent<xy::Transform>().getPosition()))
                {
                    if (found.getComponent<Actor>().id == Actor::Boat)
                    {
                        if (found.getComponent<Boat>().treasureCount > 0
                            && found.getComponent<Boat>().playerNumber != entity.getComponent<Player>().playerNumber)
                        {
                            bot.targetEntity = found;
                            bot.path.clear();
                            bot.pathRequested = false;
                            bot.targetEntity = found;
                            bot.targetPoint = found.getComponent<xy::Transform>().getPosition();
                            bot.state = Bot::State::Targeting;
                            bot.targetType = Bot::Target::Treasure;
                            return;
                        }
                    }
                    else
                    {
                        //skip any patches already dug, or have our own booby trap
                        const auto& patch = found.getComponent<WetPatch>();
                        if (patch.state != WetPatch::OneHundred
                            && patch.owner != entity.getComponent<Actor>().id)
                        {
                            const auto& patchTx = found.getComponent<xy::Transform>();

                            bot.path.clear();
                            bot.targetPoint = patchTx.getWorldPosition() + sf::Vector2f(Global::TileSize / 2.f, Global::TileSize / 2.f);

                            //we don't want to end up right on top of the wet patch
                            if (position.x > bot.targetPoint.x)
                            {
                                bot.targetPoint.x += Global::TileSize;
                            }
                            else
                            {
                                bot.targetPoint.x -= Global::TileSize;
                            }
                            bot.targetPoint.y += 2.f;

                            bot.targetEntity = found;
                            bot.targetType = Bot::Target::Hole;
                            bot.state = Bot::State::Digging;

                            requestPath(entity);

                            //drop the light if we're carrying it
                            if (entity.getComponent<Carrier>().carryFlags & (Carrier::Torch))
                            {
                                bot.inputMask |= InputFlag::CarryDrop;
                            }
                            return;
                        }
                    }
                }
            }
        }
    }
}

void BotSystem::updateDigging(xy::Entity entity, float dt)
{
    if (inSea(entity))
    {
        return;
    }

    auto& bot = entity.getComponent<Bot>();

    if (tooFar(entity))
    {
        bot.resetState();
        bot.sweepTimer = SweepTime;
        return;
    }

    //follow path to patch if path exists
    if (!bot.path.empty())
    {
        bot.pathRequested = false;
        followPath(entity);
    }
    else if (!bot.pathRequested)
    {
        //drop the light if we're carrying it
        if (entity.getComponent<Carrier>().carryFlags & Carrier::Torch)
        {
            bot.inputMask |= InputFlag::CarryDrop;
        }
        
        //we're *probably* somewhere near our destination
        //although path finding can go odd
        auto pos = entity.getComponent<xy::Transform>().getPosition();
        auto direction = bot.targetPoint - pos;
        auto len2 = xy::Util::Vector::lengthSquared(direction);

        if (len2 > 50.f)
        {
            move(pos, bot.targetPoint, bot);

            //we might have collided with something so move away and await new path
            const auto & collision = entity.getComponent<CollisionComponent>();
            for (auto i = 0u; i < collision.manifoldCount; ++i)
            {
                if (collision.manifolds[i].ID == ManifoldID::Terrain)
                {
                    bot.path.clear();
                    bot.path.push_back(entity.getComponent<xy::Transform>().getPosition() + (collision.manifolds[i].normal * 32.f));
                    break;
                }
            }

            if (len2 > 1024.f && bot.path.empty()) //may no longer be empty if collision
            {
                requestPath(entity);
            }
        }
        else
        {
            //reset the target point now we're in position
            bot.targetPoint = entity.getComponent<xy::Transform>().getPosition();

            //face the right direction
            sf::FloatRect localBounds(0.f, 0.f, Global::TileSize, Global::TileSize);
            auto worldBounds = bot.targetEntity.getComponent<xy::Transform>().getWorldTransform().transformRect(localBounds);

            auto& player = entity.getComponent<Player>();
            sf::Vector2f offset;
            switch (player.sync.direction)
            {
            default: break;
            case Player::Up:
                offset.y = -TestOffset;
                break;
            case Player::Down:
                offset.y = TestOffset;
                break;
            case Player::Left:
                offset.x = -TestOffset;
                break;
            case Player::Right:
                offset.x = TestOffset;
                break;
            }

            auto testPoint = entity.getComponent<xy::Transform>().getPosition() + offset;
            if (!worldBounds.contains(testPoint))
            {
                auto diff = (sf::Vector2f(worldBounds.left, worldBounds.top) + sf::Vector2f(Global::TileSize / 2.f, Global::TileSize / 2.f))
                    - entity.getComponent<xy::Transform>().getPosition();
                moveToPoint(diff, bot);
            }

            //dig the hole!
            const auto& inventory = entity.getComponent<Inventory>();
            if (inventory.weapon == Inventory::Shovel)
            {
                bot.buttonTimer -= dt;
                if (bot.buttonTimer < 0)
                {
                    bot.inputMask |= InputFlag::Action;
                    bot.buttonTimer = 0.8f;
                }
            }
            else
            {
                bot.desiredWeapon = Inventory::Shovel;
            }

            //if the hole was dug go back to searching
            //and reset the target entity
            if (bot.targetEntity.getComponent<WetPatch>().state == WetPatch::OneHundred)
            {
                bot.state = Bot::State::Searching;
                bot.targetEntity = {};
                bot.targetType = Bot::Target::None;
                bot.path.clear();

                //force an immediate sweep to look for items
                bot.sweepTimer = SweepTime;
            }
        }
    }

    if (entity.getComponent<Inventory>().health < bot.previousHealth)
    {
        bot.pushState(); //resume digging if we can

        //something hurt us!
        bot.state = Bot::State::Searching;
        bot.targetEntity = {};
        bot.targetType = Bot::Target::None;
        bot.path.clear();
        bot.sweepTimer = SweepTime; //force a sweep and hope we find the attacker
    }
}

void BotSystem::updateFighting(xy::Entity entity, float dt)
{
    if (inSea(entity))
    {
        return;
    }

    auto& bot = entity.getComponent<Bot>();

    /*if (tooFar(entity))
    {
        bot.resetState();
        bot.sweepTimer = SweepTime;
        return;
    }*/

    bot.fightTimer += dt;
    if (bot.fightTimer > MaxMoveTime)
    {
        bot.popState();
        return;
    }
    //if we're not losing health then we're probably not fighting
    const auto& inventory = entity.getComponent<Inventory>();
    if (bot.previousHealth > inventory.health)
    {
        bot.fightTimer = 0.f;
    }

    //find distance to target (or target valid in the case of skeletons)  
    if (!bot.targetEntity.isValid()
        || !bot.targetEntity.hasComponent<Actor>())
    {
        LOG("Invalid target", xy::Logger::Type::Info);
        bot.popState(); //return to what we were doing
        return;
    }

    auto targetActor = bot.targetEntity.getComponent<Actor>();
    auto actorID = targetActor.id;
    bool validActor = false;
    bool validState = true;
    switch (actorID)
    {
    default: break;
    case Actor::PlayerOne:
    case Actor::PlayerTwo:
    case Actor::PlayerThree:
    case Actor::PlayerFour:
        if (bot.targetEntity.hasComponent<Player>())
        {
            validState = bot.targetEntity.getComponent<Player>().sync.state != Player::Dead;
        }
    case Actor::Skeleton:
    case Actor::Barrel:
        validActor = true;
        break;
    }

    if (!(validActor && validState))
    {
        bot.popState();
        bot.sweepTimer = SweepTime;
        return;
    }

    //if target blocked then quit and drop back to last state (sweep should find any dropped items)
    auto position = entity.getComponent<xy::Transform>().getPosition();
    auto targetPosition = bot.targetEntity.getComponent<xy::Transform>().getPosition();// -position;
    
    //for some reason enabling this causes a strange exception
    //where 'this' pointer becomes invalid. Sounds like a horrible
    //memory corruption bug...
    /*if (m_pathFinder.rayTest(position, targetPosition))
    {
        LOG(ActorNames[entity.getComponent<Actor>().id] + ": enemy was obscured", xy::Logger::Type::Info);
        bot.popState();
        return;
    }*/
    auto direction = targetPosition - position;
    auto len2 = xy::Util::Vector::lengthSquared(direction);
    if (len2 > 4096 && actorID != Actor::Barrel
        && bot.path.empty()) //approx 2 tile radius
    {
        LOG(ActorNames[entity.getComponent<Actor>().id] + ": enemy was too far", xy::Logger::Type::Info);
        bot.popState();
        return;
    }


    auto fightingDistance = IdealSwordFightingDistance;
    auto minFightingDistance = MinSwordFightingDistance;
    if (inventory.ammo == 0 || inventory.weapon == Inventory::Shovel)
    {
        bot.desiredWeapon = Inventory::Sword;
    }
    else
    {
        bot.desiredWeapon = Inventory::Pistol;
        fightingDistance = IdealPistolFightingDistance;
        minFightingDistance = MinPistolFightingDistance;
    }

    //else try to move within fighting distance (but not on top of target)
    if (len2 > fightingDistance)
    {
        moveToPoint(direction, bot);
    }
    else
    {
        //face enemy
        auto playerDir = entity.getComponent<Player>().sync.direction;
        bool canStrafe = true;
        bool moveX = true;
        if (std::abs(direction.y) > std::abs(direction.x))
        {
            //face up or down
            if (direction.y < 0 && playerDir != Player::Up)
            {
                bot.inputMask |= InputFlag::Up;
                canStrafe = false;
            }
            else if(direction.y > 0 && playerDir != Player::Down)
            {
                bot.inputMask |= InputFlag::Down;
                canStrafe = false;
            }
        }
        else
        {
            //face left or right
            if (direction.x < 0 && playerDir != Player::Left)
            {
                bot.inputMask |= InputFlag::Left;
                canStrafe = false;
            }
            else if(direction.x > 0 && playerDir != Player::Right)
            {
                bot.inputMask |= InputFlag::Right;
                canStrafe = false;
            }
            moveX = false; //y distance is shorter
        }

        if (canStrafe)
        {
            //stay facing enemy
            bot.inputMask |= InputFlag::Strafe;
        }

        //and maintain our distance
        if (len2 < minFightingDistance)
        {
            moveToPoint(-direction, bot);
            bot.inputMask |= InputFlag::Strafe;
        }
        else
        {
            if (moveX)
            {
                direction.y = 0.f;
            }
            else
            {
                direction.x = 0.f;
            }
            moveToPoint(direction, bot);
        }
    }

    
    //switch weapon or hammer the weapon button :)
    bot.buttonTimer -= dt;
    if (bot.buttonTimer < 0)
    {
        if (bot.desiredWeapon != inventory.weapon)
        {
            bot.inputMask |= InputFlag::WeaponNext;
        }
        else
        {
            bot.inputMask |= InputFlag::Action;
        }
        bot.buttonTimer = 0.6f;
    }
    

    //bail if collision
    const auto& collision = entity.getComponent<CollisionComponent>();
    for (auto i = 0u; i < collision.manifoldCount; ++i)
    {
        if (collision.manifolds[i].ID == ManifoldID::Terrain)
        {
            bot.resetState();
            bot.path.push_back((collision.manifolds[i].normal * Global::TileSize) + position);
            return;
        }
        else if (collision.manifolds[i].ID == ManifoldID::SpawnArea)
        {
            if (collision.manifolds[i].otherEntity.getComponent<SpawnArea>().parent.getComponent<Boat>().playerNumber
                != entity.getComponent<Player>().playerNumber)
            {
                bot.resetState();
                bot.path.push_back((collision.manifolds[i].normal * Global::TileSize) + position);
                return;
            }
        }
    }


    //run away if health too low
    //if (inventory.health < (Inventory::MaxHealth / 3)
    //    && actorID != Actor::Barrel) //these aren't scary. Usually
    //{
    //    auto otherHealth = Inventory::MaxHealth;
    //    if (targetActor.serverID == targetActor.entityID)
    //    {
    //        //must be a server ent - client ents don't have an inventory.
    //        otherHealth = bot.targetEntity.getComponent<Inventory>().health;
    //    }
    //
    //    //flee mode so we can't fight again until certain conditions are met
    //    if (inventory.ammo == 0
    //        && inventory.health < otherHealth)
    //    {
    //        LOG(ActorNames[entity.getComponent<Actor>().id] + ": Run away!!", xy::Logger::Type::Info);
    //        bot.pushState();
    //        bot.resetState();
    //
    //        //look for the furthest exit (to give best chance to run)
    //        bot.fleeing = true;
    //        bot.fleeTimer = 0.f;
    //
    //        auto pos = position;
    //        if (pos.x > (Global::IslandSize.x / 2.f))
    //        {
    //            pos.x = 0.f;
    //        }
    //        else
    //        {
    //            pos.x = Global::IslandSize.x;
    //        }
    //        bot.targetPoint = pos;
    //        requestPath(entity);
    //
    //        return;
    //    }
    //}
}

void BotSystem::updateCapturing(xy::Entity entity)
{
    auto& bot = entity.getComponent<Bot>();

    if (!bot.path.empty())
    {
        bot.pathRequested = false;
        followPath(entity);
    }
    else if (!bot.pathRequested)
    {
        //we must be near the boat by now
        //walk to target point
        auto dir = /*bot.targetPoint*/Global::BoatPositions[entity.getComponent<Player>().playerNumber] - entity.getComponent<xy::Transform>().getPosition();
        auto len2 = xy::Util::Vector::lengthSquared(dir);
        
        if ((len2 > /*4096.f*/9120.f) && !bot.pathRequested)//approx 3 tile radius
        {
            //something went wrong and we need a new path to spawn
            bot.targetPoint = entity.getComponent<Player>().spawnPosition;
            //bot.targetPoint.y -= Global::PlayerSpawnOffset;
            bot.targetPoint -= Global::SpawnOffsets[entity.getComponent<Player>().playerNumber];
            requestPath(entity);
            LOG("looking for home!", xy::Logger::Type::Info);
        }
        else if (len2 > 100.f)
        {
            moveToPoint(dir, bot);
        }
    }

    //check if we're no longer holding the treasure and return to searching
    if ((entity.getComponent<Carrier>().carryFlags & Carrier::Treasure) == 0)
    {
        bot.resetState();

        //move away from the boat
        auto position = entity.getComponent<xy::Transform>().getPosition();
        if (position.y > bot.targetPoint.y)
        {
            bot.targetPoint.y += Global::TileSize * 2.f;
        }
        else
        {
            bot.targetPoint.y -= Global::TileSize * 2.f;
        }
        bot.path.push_back(bot.targetPoint);
        LOG(ActorNames[entity.getComponent<Actor>().id] + ": Dropped treasure in boat!", xy::Logger::Type::Info);
    }

    //we might have collided with something so move away and await new path
    const auto & collision = entity.getComponent<CollisionComponent>();
    for (auto i = 0u; i < collision.manifoldCount; ++i)
    {
        if (collision.manifolds[i].ID == ManifoldID::Terrain)
        {
            bot.path.clear();
            bot.path.push_back(entity.getComponent<xy::Transform>().getPosition() + (collision.manifolds[i].normal * 32.f));
            break;
        }
    }
}

void BotSystem::updateTargeting(xy::Entity entity, float dt)
{
    if (inSea(entity))
    {
        return;
    }

    auto& bot = entity.getComponent<Bot>();

    /*if (tooFar(entity))
    {
        bot.resetState();
        bot.sweepTimer = SweepTime;
        return;
    }*/

    //if we managed to pick something up see what it is
    //then switch to the appropriate state
    auto flags = entity.getComponent<Carrier>().carryFlags;
    if (flags /*& (Carrier::Treasure | Carrier::Torch)*/)
    {
        bot.wantsGrab = false;
        if (flags & Carrier::Treasure)
        {
            bot.targetEntity = {};
            bot.targetType = Bot::Target::None;
            bot.state = Bot::State::Capturing;
            bot.path.clear();
            bot.pathRequested = false;

            LOG(ActorNames[entity.getComponent<Actor>().id] + ": picked up treasure!", xy::Logger::Type::Info);
            bot.targetPoint = entity.getComponent<Player>().spawnPosition;
            //bot.targetPoint.y -= Global::PlayerSpawnOffset;
            bot.targetPoint -= Global::SpawnOffsets[entity.getComponent<Player>().playerNumber];
            requestPath(entity);       
        }
        else
        {
            bot.itemTimer = 8.f;
            bot.popState();
        }
        return;
    }

    //if near target search area and react on target
    //type - ie pick up treasure or lights and switch state
    //drop light in favour of treasure
    //or walk over a collectible
    
    //target entity may have been removed by someone else!!
    if (!bot.targetEntity.isValid()
        || bot.targetEntity.destroyed())
    {
        bot.popState();
        return;
    }

    if (bot.targetType == Bot::Target::Light && isDay())
    {
        //give up we don't need the light any more
        bot.popState();
        return;
    }

    auto dir = bot.targetPoint//Entity.getComponent<xy::Transform>().getPosition() 
        - entity.getComponent<xy::Transform>().getPosition();
    auto len2 = xy::Util::Vector::lengthSquared(dir);
    if (len2 < /*1024.f*/512.f) //arbitrary distance somewhere close to target
    {
        if (bot.targetType == Bot::Target::Enemy)
        {
            //target probably moved, unless it was a barrel
            //in which case the next sweep will find it
            bot.popState();
            return;
        }

        if (bot.targetType == Bot::Target::Collectable)
        {
            moveToPoint(dir, bot);
            if (len2 < 32.f)
            {
                //assume we picked it up and go seeking again
                bot.popState();

                LOG("picked up collectible!", xy::Logger::Type::Info);
            }
        }
        else
        {
            if (bot.wantsGrab)
            {
                //we missed last time so move a bit closer
                moveToPoint(dir * 2.f, bot);
                bot.wantsGrab = false;
            }
            else
            {
                //face target
                dir = bot.targetEntity.getComponent<xy::Transform>().getPosition()
                    - entity.getComponent<xy::Transform>().getPosition();

                //TODO if we walked towards the target we should already be facing
                //the correct way - so if the grab fails we can try reorientating
                //next loop (above ^^)

                //auto playerDir = entity.getComponent<Player>().sync.direction;
                //if (std::abs(dir.y) > std::abs(dir.x))
                //{
                //    //face up or down
                //    if (dir.y < 0 && playerDir != Player::Up)
                //    {
                //        bot.inputMask |= InputFlag::Up;
                //    }
                //    else if (dir.y > 0 && playerDir != Player::Down)
                //    {
                //        bot.inputMask |= InputFlag::Down;
                //    }
                //}
                //else
                //{
                //    //face left or right
                //    if (dir.x < 0 && playerDir != Player::Left)
                //    {
                //        bot.inputMask |= InputFlag::Left;
                //    }
                //    else if (dir.x > 0 && playerDir != Player::Right)
                //    {
                //        bot.inputMask |= InputFlag::Right;
                //    }
                //}

                //try pick up. The next loop we check if
                //we're carrying anything (above^^) and switch state
                bot.buttonTimer -= dt;
                if (bot.buttonTimer < 0)
                {
                    bot.inputMask |= InputFlag::CarryDrop;
                    bot.wantsGrab = true;
                    bot.buttonTimer = 0.6f;
                }
            }
        }
    }
    //else
    {

        //if no path find one to target
        if (bot.path.empty() && !bot.pathRequested)
        {
            requestPath(entity);
        }

        //else follow path
        else if (!bot.path.empty())
        {
            bot.pathRequested = false;
            followPath(entity);
        }
    }
}

bool BotSystem::inSea(xy::Entity entity)
{
    auto& bot = entity.getComponent<Bot>();

    //if we're in the water for some reason, path to land
    if (entity.getComponent<CollisionComponent>().water > 0.24f
        && bot.path.empty())
    {
        const auto position = entity.getComponent<xy::Transform>().getPosition();
        sf::Vector2f target = position;
        const float minBounds = Global::TileSize * 2.f;// 20.f;
        if (position.x < minBounds)
        {
            target.x = 64.f;
        }
        else if (position.x > (Global::IslandSize.x - minBounds))
        {
            target.x = Global::IslandSize.x - 64.f;
        }

        if (position.y < minBounds)
        {
            target.y = 64.f;
        }
        else if (position.y > (Global::IslandSize.y - minBounds))
        {
            target.y = Global::IslandSize.y - 64.f;
        }

        bot.resetState();
        bot.targetPoint = target;
        bot.path.push_back(target);
        bot.sweepTimer = -14.f;
        bot.fleeing = true;
        std::cout << "escaping water\n";
        return true;
    }

    return false;
}

bool BotSystem::tooFar(xy::Entity entity)
{
    //this is called to check that, for example, popping a state
    //after fleeing or fighting hasn't left us with a previous
    //target which is now miles away

    auto pos = entity.getComponent<xy::Transform>().getPosition();
    auto target = entity.getComponent<Bot>().targetPoint;

    static const float MaxDist = (Global::IslandSize.x / 4.f) * (Global::IslandSize.x / 4.f);
    return (xy::Util::Vector::lengthSquared(target - pos) > MaxDist);
}

void BotSystem::wideSweep(xy::Entity entity)
{
    auto& bot = entity.getComponent<Bot>();

    //drop the light if we're in day time again
    if (isDay() && entity.getComponent<Carrier>().carryFlags & Carrier::Torch)
    {
        bot.inputMask |= InputFlag::CarryDrop;
    }

    if (bot.state != Bot::State::Fighting && bot.state != Bot::State::Capturing)
    {
        auto position = entity.getComponent<xy::Transform>().getPosition();

        sf::FloatRect bounds;
        if (isDay())
        {
            //large visible area, approx what the player sees on screen (without accounting for perspective)
            bounds = DaySearchArea;
        }
        else
        {
            //only see approx light area
            bounds = NightSearchArea;
        }

        bounds.left += position.x;
        bounds.top += position.y;

        const auto& tree = getScene()->getSystem<xy::DynamicTreeSystem>();
        auto nearby = tree.query(bounds, QuadTreeFilter::BotQuery);

        //sort here to make sure treasure is prioritised over lights
        std::sort(nearby.begin(), nearby.end(), 
            [](const xy::Entity& a, const xy::Entity& b) 
        {
            return a.getComponent<Actor>().id < b.getComponent<Actor>().id;
        });

        for (auto ent : nearby)
        {
            if (ent == entity)
            {
                continue;
            }

            auto targetPosition = ent.getComponent<xy::Transform>().getPosition();
            if (m_pathFinder.rayTest(position, targetPosition))
            {
                //ignore if no LOS
                continue;
            }

            const auto& actor = ent.getComponent<Actor>();
            
            SweepResult result = SweepResult::None;
            switch (actor.id)
            {
            default: break;
            case Actor::Skeleton:
            case Actor::Barrel:
            case Actor::PlayerOne:
            case Actor::PlayerTwo:
            case Actor::PlayerThree:
            case Actor::PlayerFour:
            {
                //auto targetPosition = ent.getComponent<xy::Transform>().getPosition();
                auto direction = targetPosition - position;
                auto len2 = xy::Util::Vector::lengthSquared(direction);

                if (len2 < MaxFightDistance/* && !m_pathFinder.rayTest(position, targetPosition)*/)
                {
                    //only target if close enough and not obstructed
                    result = SweepResult::Fight;
                }
                else if (bot.state != Bot::State::Targeting
                    && actor.id != Actor::Skeleton)
                {
                    //try path to target if not already doing so
                    result = SweepResult::Target;
                }
                else
                {
                    //we already have better things to do that seek out an enemy
                    continue;
                }
            }
                break;
            case Actor::Treasure:
            case Actor::Coin:
            case Actor::Ammo:
            case Actor::Food:
            case Actor::Boat:
            case Actor::SkullItem:
            case Actor::FlareItem:
            case Actor::DecoyItem:
            case Actor::MineItem:
                result = SweepResult::Target;
                break;
            }

            //fighting is higher priority than getting items
            if (result == SweepResult::Fight)
            {
                if (bot.state == Bot::State::Digging)
                {
                    //only change state if enemy is near
                    //auto targetPosition = ent.getComponent<xy::Transform>().getPosition();
                    auto direction = targetPosition - position;
                    auto len2 = xy::Util::Vector::lengthSquared(direction);
                    if (len2 > 4096) //approx 2 tile radius
                    {
                        return;
                    }
                }

                if (!bot.fleeing)
                {
                    //if carrying something, drop it
                    if (entity.getComponent<Carrier>().carryFlags & (Carrier::Torch | Carrier::Treasure))
                    {
                        bot.inputMask |= InputFlag::CarryDrop;
                    }

                    bot.pushState();

                    bot.path.clear();
                    bot.pathRequested = false;
                    bot.targetEntity = ent;
                    bot.targetType = Bot::Target::Enemy;
                    bot.state = Bot::State::Fighting;
                    bot.movementTimer = 0.f;
                    bot.fightTimer = 0.f;
                    LOG(ActorNames[entity.getComponent<Actor>().id] + ": Found enemy - " + ActorNames[ent.getComponent<Actor>().id], xy::Logger::Type::Info);
                }
                return;
            }
            else if (result == SweepResult::Target
                && bot.state != Bot::State::Digging)
            {
                //TODO barrels / food should increase in priority when health is low                

                if (ent == bot.targetEntity)
                {
                    //already targeted, skip
                    return;
                }

                //ignore ammo if inventory full
                if (actor.id == Actor::ID::Ammo
                    && entity.getComponent<Inventory>().ammo == Inventory::MaxAmmo)
                {
                    continue;
                }

                //skip treasure if it's already carried by someone else
                if (actor.id == Actor::ID::Treasure 
                    && ent.getComponent<Carriable>().carried)
                {
                    LOG("Treasure already carried", xy::Logger::Type::Info);
                    continue;
                }

                //ignore health
                if (actor.id == Actor::ID::Food
                    && entity.getComponent<Inventory>().health > Inventory::MaxHealth - 5)
                {
                    bot.sweepTimer = -4.f; //give us enough chance to move away and not keep finding the health item
                    return;
                }

                //drop lantern if carrying 
                if (actor.id == Actor::ID::Treasure &&
                    entity.getComponent<Carrier>().carryFlags & (Carrier::Torch))
                {
                    bot.inputMask |= InputFlag::CarryDrop;
                }

                if (actor.id == Actor::Boat &&
                    (ent.getComponent<Boat>().treasureCount == 0 || ent.getComponent<Boat>().playerNumber == entity.getComponent<Player>().playerNumber))
                {
                    //ignore empty boats and don't try stealing from ourself
                    continue;
                }

                //skip items if we're carrying something or they are carried by someone else
                if ((actor.id == Actor::FlareItem || actor.id == Actor::DecoyItem || actor.id == Actor::SkullItem || actor.id == Actor::MineItem)
                    && (entity.getComponent<Carrier>().carryFlags != 0 || ent.getComponent<Carriable>().carried))
                {
                    continue;
                }

                //if we're aleady targeting only chose new target if it's closer
                //and more valuable (based on actor id) and already passed get clauses
                if (bot.state == Bot::State::Targeting)
                {
                    auto pos = entity.getComponent<xy::Transform>().getPosition();
                    auto targetPosition = ent.getComponent<xy::Transform>().getPosition();

                    auto newDist = xy::Util::Vector::lengthSquared(targetPosition - pos);
                    auto currDist = xy::Util::Vector::lengthSquared(bot.targetPoint - pos);

                    if (newDist > currDist
                        || (bot.targetEntity.hasComponent<Actor>() && actor.id > bot.targetEntity.getComponent<Actor>().id))
                    {
                        continue;
                    }
                }

                //plot path to collectable
                LOG(ActorNames[entity.getComponent<Actor>().id] + ": Found " + ActorNames[actor.id] + " attempting to path!", xy::Logger::Type::Info);

                bot.pushState();

                bot.targetEntity = ent;
                bot.path.clear();
                bot.pathRequested = false;
                bot.targetEntity = ent;
                bot.targetPoint = ent.getComponent<xy::Transform>().getWorldPosition();
                bot.state = Bot::State::Targeting;

                switch (actor.id)
                {
                default: 
                    bot.targetType = Bot::Target::Item;
                    break;
                case Actor::Treasure:
                case Actor::Boat:
                    bot.targetType = Bot::Target::Treasure;
                    break;
                case Actor::Food:
                case Actor::Ammo:
                case Actor::Coin:
                    bot.targetType = Bot::Target::Collectable;
                    break;
                case Actor::Skeleton:
                case Actor::Barrel:
                case Actor::PlayerOne:
                case Actor::PlayerTwo:
                case Actor::PlayerThree:
                case Actor::PlayerFour:
                    bot.targetType = Bot::Target::Enemy;
                    break;
                }

                //if we're already close skip path finding
                if ((xy::Util::Vector::lengthSquared(bot.targetPoint - position) < MinTargetArea))
                {
                    bot.path.push_back(bot.targetPoint);
                }

                return;
            }
            //if dark and not capturing look for lights
            else if (!isDay() && actor.id == Actor::ID::Lantern && bot.state == Bot::State::Searching)
            {
                if ((entity.getComponent<Carrier>().carryFlags & (Carrier::Torch | Carrier::Treasure)) == 0)
                {
                    if (!ent.getComponent<Carriable>().carried && bot.targetType != Bot::Target::Treasure)
                    {
                        //LOG("Ooh a light!", xy::Logger::Type::Info);
                        bot.pushState();

                        std::uniform_int_distribution<int> dist(-1, 1);

                        bot.targetEntity = ent;
                        bot.path.clear();
                        bot.pathRequested = false;
                        bot.targetEntity = ent;
                        bot.targetPoint = ent.getComponent<xy::Transform>().getWorldPosition();
                        bot.targetPoint.x += dist(m_rndEngine) * 6.f;
                        bot.targetType = Bot::Target::Light;
                        bot.state = Bot::State::Targeting;
                        return;
                    }
                }
            }
        }
    }
}

void BotSystem::followPath(xy::Entity entity)
{
    auto& bot = entity.getComponent<Bot>();
    auto position = entity.getComponent<xy::Transform>().getPosition();
    
    //check conditions such as being too near another player's boat
    //or walked into something
    const auto& collision = entity.getComponent<CollisionComponent>();
    for (auto i = 0u; i < collision.manifoldCount; ++i)
    {
        auto manID = collision.manifolds[i].ID;
        if (manID == ManifoldID::Terrain
            || manID == ManifoldID::Boat)
        {
            bot.path.clear();
            bot.path.push_back((collision.manifolds[i].normal * Global::TileSize) + position);
            break;
        }
        else if (manID == ManifoldID::SpawnArea)
        {
            if (collision.manifolds[i].otherEntity.getComponent<SpawnArea>().parent.getComponent<Boat>().playerNumber
                != entity.getComponent<Player>().playerNumber)
            {
                bot.path.clear();
                bot.path.push_back((collision.manifolds[i].normal * Global::TileSize) + position);
            }
        }
    }
    
    //see how close we are to next node - player movement is pretty
    //fast so sometimes it's possible for the bot to move greater
    //than the minimum distance - therefore we reduce acceleration as
    //we get nearer
    auto dest = bot.path.back();
    auto len2 = xy::Util::Vector::lengthSquared(dest - position);
    bot.acceleration = std::min(0.85f, len2 / MinDistance);

    if (len2 < MinDistance)
    {
        bot.path.pop_back();
        if (!bot.path.empty())
        {
            dest = bot.path.back();
            bot.movementTimer = 0.f;
            //std::cout << "Switched to next point\n";
        }
        else
        {
            //std::cout << "completed path!\n";
            return;
        }
    }

    if (!bot.path.empty() &&
        bot.movementTimer > MaxMoveTime)
    {
        //we've been stuck on this node too long
        auto newDest = position - (bot.path.back() - position);
        bot.path.clear();
        bot.path.push_back(newDest);
    }

    move(position, dest, bot);
}

void BotSystem::requestPath(xy::Entity entity)
{
    auto& bot = entity.getComponent<Bot>();
    auto position = entity.getComponent<xy::Transform>().getPosition();

    //clamp the start position to island bounds so we generate
    //a valid path when the bot is in the sea
    position.x = xy::Util::Math::clamp(position.x, 0.f, Global::IslandSize.x);
    position.y = xy::Util::Math::clamp(position.y, 0.f, Global::IslandSize.y);

    sf::Vector2i start(position / Global::TileSize);
    sf::Vector2i end(bot.targetPoint / Global::TileSize);

    static const float minDist = (Global::TileSize * 2.f) * (Global::TileSize * 2.f);

    //if (start == end)
    if(xy::Util::Vector::lengthSquared(position - bot.targetPoint) < minDist
        && !m_pathFinder.rayTest(position, bot.targetPoint))
    {
        //head straight to target
        bot.path.push_back(bot.targetPoint);
    }
    else
    {
        m_pathFinder.plotPathAsync(start, end, bot.path);
        bot.pathRequested = true;
    }
        
    bot.movementTimer = 0.f;
}

void BotSystem::moveToPoint(sf::Vector2f dir, Bot& bot)
{
    static const float minMove = 1.5f;

    if (dir.x > minMove)
    {
        bot.inputMask |= InputFlag::Right;
    }
    else if (dir.x < -minMove)
    {
        bot.inputMask |= InputFlag::Left;
    }
    if (dir.y < -minMove)
    {
        bot.inputMask |= InputFlag::Up;
    }
    else if (dir.y > minMove)
    {
        bot.inputMask |= InputFlag::Down;
    }
    bot.acceleration = 0.85f;
}

void BotSystem::move(sf::Vector2f from, sf::Vector2f to, Bot& bot)
{
    if (to.x < (from.x - DeadZone))
    {
        bot.inputMask |= InputFlag::Left;
    }
    else if (to.x >(from.x + DeadZone))
    {
        bot.inputMask |= InputFlag::Right;
    }

    if (to.y < (from.y - DeadZone))
    {
        bot.inputMask |= InputFlag::Up;
    }
    else if (to.y >(from.y + DeadZone))
    {
        bot.inputMask |= InputFlag::Down;
    }
}

void BotSystem::onEntityAdded(xy::Entity entity)
{
    auto getRandomInt = [&](int start, int end)->int
    {
        std::uniform_int_distribution<int> dist(start, end);
        return dist(m_rndEngine);
    };

    entity.getComponent<Bot>().pointIndex = getRandomInt(0, m_destinationPoints.size());
    entity.getComponent<Bot>().indexStride = getRandomInt(1, 3);
    entity.getComponent<Bot>().targetPoint = m_destinationPoints[getRandomInt(0, m_destinationPoints.size())];
}