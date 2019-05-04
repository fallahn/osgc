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

#include "Human.hpp"
#include "Navigation.hpp"
#include "CollisionTypes.hpp"
#include "RadarItemSystem.hpp"
#include "MessageIDs.hpp"
#include "CollisionUtil.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

#include <algorithm>

namespace
{
    const sf::Vector2f SeekSize(256.f, 256.f);

    const sf::Time SpawnTime = sf::seconds(2.8f);
    const std::size_t SpawnsPerPoint = 1;
    const std::size_t MaxSpawns = 10;

    const float MaxWallDistance = 32.f;
    const float MaxWallDistSqr = MaxWallDistance * MaxWallDistance;

    const float NodeDistanceSqr = (Node::Bounds[2]) * (Node::Bounds[2]);
}

HumanSystem::HumanSystem(xy::MessageBus& mb, const SpriteArray& sprites, std::int32_t difficulty)
    : xy::System(mb, typeid(HumanSystem)),
    m_sprites   (sprites),
    m_spawnIndex(0),
    m_spawnCount(0),
    m_humanCount(Human::NumberPerRound / difficulty),
    m_difficulty(difficulty)
{
    requireComponent<xy::Transform>();
    requireComponent<Human>();
    requireComponent<Navigator>();
    requireComponent<xy::BroadphaseComponent>();
}

//public
void HumanSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        const auto& human = entity.getComponent<Human>();

        //update movement
        tx.move(human.velocity * human.speed * dt);
        tx.rotate(xy::Util::Math::shortestRotation(tx.getRotation(), xy::Util::Vector::rotation(human.velocity)) * 10.f * dt);

        switch (human.state)
        {
        default: break;
        case Human::State::Normal:
            updateNormal(entity);
            break;
        case Human::State::Seeking:
            updateSeeking(entity);
            break;
        case Human::State::Scared:
            updateScared(entity);
            break;
        }

        updateCollision(entity);

#ifdef XY_DEBUG
        /*switch (human.state)
        {
        default: break;
        case Human::State::Seeking:
            entity.getComponent<xy::Sprite>().setColour(sf::Color::Blue);
            break;
        case Human::State::Normal:
            entity.getComponent<xy::Sprite>().setColour(sf::Color::Green);
            break;
        case Human::State::Scared:
            entity.getComponent<xy::Sprite>().setColour(sf::Color::Red);
            break;
        }*/
#endif
    }

    //check if OK to spawn new human
    if (entities.size() < MaxSpawns
        && m_humanCount > 0)
    {
        if (m_spawnClock.getElapsedTime() > SpawnTime)
        {
            m_spawnClock.restart();
            spawnHuman();

            m_spawnCount = (m_spawnCount + 1) % SpawnsPerPoint;
            if (m_spawnCount == 0)
            {
                m_spawnIndex = (m_spawnIndex + 1) % m_spawnPoints.size();
            }
        }
    }
    else
    {
        m_spawnClock.restart();
    }
}

void HumanSystem::addSpawn(sf::Vector2f v)
{
    m_spawnPoints.push_back(v);
    std::shuffle(m_spawnPoints.begin(), m_spawnPoints.end(), xy::Util::Random::rndEngine);
}

void HumanSystem::clearSpawns()
{
    m_spawnPoints.clear();
    m_spawnIndex = 0;
    m_spawnCount = 0;
    m_humanCount = Human::NumberPerRound / m_difficulty;
}

//private
void HumanSystem::spawnHuman()
{
    auto entity = getScene()->createEntity();
    auto& tx = entity.addComponent<xy::Transform>();
    tx.setPosition(m_spawnPoints[m_spawnIndex]);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 2);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Human];
    entity.addComponent<xy::SpriteAnimation>().play(1);
    auto bounds = m_sprites[SpriteID::Human].getTextureBounds();
    tx.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    tx.setScale(4.f, 4.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionBox::Filter::Human);
    entity.addComponent<CollisionBox>().type = CollisionBox::NPC;
    sf::Vector2f velocity =
    {
        xy::Util::Random::value(-1.f, 1.f),
        xy::Util::Random::value(-1.f, 1.f)
    };
    entity.addComponent<Human>().velocity = xy::Util::Vector::normalise(velocity);
    entity.addComponent<Navigator>();

    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float)
    {
        if (!ConstVal::MapArea.contains(e.getComponent<xy::Transform>().getPosition()))
        {
            getScene()->destroyEntity(e);
            xy::Logger::log("Destroyed human out of bounds", xy::Logger::Type::Info);
        }
    };

    auto* msg = postMessage<HumanEvent>(MessageID::HumanMessage);
    msg->type = HumanEvent::Spawned;
    msg->position = tx.getPosition();
    msg->rotation = tx.getRotation();

    m_humanCount--;

    //apparently there's no radar icon for humans...
    /*auto humanEntity = entity;
    entity = getScene()->createEntity();
    entity.addComponent<xy::Transform>().setPosition(-100.f, -100.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
    entity.addComponent<xy::Sprite>() = m_sprites[];*/
}

void HumanSystem::updateNormal(xy::Entity entity)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& human = entity.getComponent<Human>();
    
    auto& nav = entity.getComponent<Navigator>();
    const auto& path = nav.target.getComponent<Node>().path;

    auto dir = tx.getPosition() - path[nav.pathIndex];
    if (xy::Util::Vector::lengthSquared(dir) < NodeDistanceSqr)
    {
        nav.pathIndex++;
        if (nav.pathIndex < path.size())
        {
            human.velocity = xy::Util::Vector::normalise(path[nav.pathIndex] - tx.getPosition());
        }
        else
        {
            human.state = Human::State::Seeking;
        }
    }

    //check for aliens and switch to scared state
    sf::FloatRect searchArea(tx.getPosition(), SeekSize);
    searchArea.left -= (SeekSize.x / 2.f);
    searchArea.top -= (SeekSize.y / 2.f);

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Filter::Alien);
    if (!nearby.empty())
    {
        human.state = Human::State::Scared;
    }
}

void HumanSystem::updateSeeking(xy::Entity entity)
{
    //walk randomly while bouncing off walls until finding a new node target
    auto& tx = entity.getComponent<xy::Transform>();
    auto& human = entity.getComponent<Human>();

    sf::FloatRect searchArea(tx.getPosition(), SeekSize);
    searchArea.left -= (SeekSize.x / 2.f);
    searchArea.top -= (SeekSize.y / 2.f);

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Filter::Alien | CollisionBox::Filter::Navigation);
    std::shuffle(nearby.begin(), nearby.end(), xy::Util::Random::rndEngine); //increase chance of picking a different nearby node (as first always gets picked)
    for(auto e : nearby)
    {       
        //if (e != entity) //probably don't need this check....
        {
            if (e.getComponent<xy::BroadphaseComponent>().getFilterFlags() & CollisionBox::Navigation)
            {
                //TODO cast ray to found node and check for solid collision?

                //make sure this wasn't the previous node
                auto& nav = entity.getComponent<Navigator>();
                const auto& node = e.getComponent<Node>();

                if (nav.previousNode != node.ID)
                {
                    nav.previousNode = node.ID;
                    nav.target = e;
                    nav.pathIndex = 0;

                    human.velocity = node.path[0] - tx.getPosition();
                    human.velocity = xy::Util::Vector::normalise(human.velocity);

                    human.state = Human::State::Normal;

                    return;
                }
            }
            else
            {
                //must be aliens nearby, run away!
                human.state = Human::State::Scared;

                return;
            }
        }
    }
}

void HumanSystem::updateScared(xy::Entity entity)
{
    //gather nearby aliens, average the center postion, add dir to velocity and normalise
    //run faster more aliens there are
    //switch to seeking if no aliens nearby

    auto& human = entity.getComponent<Human>();
    auto& tx = entity.getComponent<xy::Transform>();
    sf::FloatRect searchArea(tx.getPosition(), SeekSize);
    searchArea.left -= (SeekSize.x / 2.f);
    searchArea.top -= (SeekSize.y / 2.f);

    auto bounds = tx.getTransform().transformRect(entity.getComponent<xy::BroadphaseComponent>().getArea());

    float count = 0.f;
    sf::Vector2f centreOfMass;
    sf::Vector2f wallVector;

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Filter::Alien | CollisionBox::Solid | CollisionBox::Filter::Navigation);
    for (auto e : nearby)
    {
        auto flags = e.getComponent<xy::BroadphaseComponent>().getFilterFlags();
        if (flags & CollisionBox::Alien)
        {
            auto otherPosition = e.getComponent<xy::Transform>().getPosition();
            centreOfMass += otherPosition;
            count++;

            sf::FloatRect otherBounds(-16.f, -16.f, 32.f, 32.f);
            otherBounds.left += otherPosition.x;
            otherBounds.top += otherPosition.y;

            //check for intersection and kill poor human :(
            if (bounds.intersects(otherBounds))
            {
                auto* msg = postMessage<HumanEvent>(MessageID::HumanMessage);
                msg->type = HumanEvent::Died;
                msg->position = tx.getPosition();
                msg->rotation = tx.getRotation();

                getScene()->destroyEntity(entity);
                return;
            }
        }
        else if (flags & CollisionBox::Navigation)
        {
            //run towards these
            //TODO don't want to get stuck though... perhaps need to 
            //do something a bit clever combining 'normal' state and running away
        }
        else
        {
            //steers human away from solid objects to stop them face planting walls
            auto otherBounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());

            auto ray = human.velocity * MaxWallDistance;

            auto result = intersects(std::make_pair(tx.getPosition(), tx.getPosition() + ray), otherBounds);
            if (result.intersects)
            {
                wallVector += xy::Util::Vector::reflect(human.velocity, result.normal);
            }
        }
    }

    if (count > 0)
    {
        centreOfMass /= count;
        human.velocity += (tx.getPosition() - centreOfMass);
        human.velocity += wallVector;
        human.velocity = xy::Util::Vector::normalise(human.velocity);
    }
    else
    {
        human.state = Human::State::Seeking;
    }
    //run faster the more aliens there are?
    //we want to make sure aliens can catch the humans....
    human.speed = Human::DefaultSpeed + ((count * 3.f) * 20.f);
}

void HumanSystem::updateCollision(xy::Entity entity)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto& human = entity.getComponent<Human>();

    //sf::FloatRect bounds = tx.getTransform().transformRect(entity.getComponent<xy::BroadphaseComponent>().getArea());
    sf::FloatRect bounds(-16.f, -16.f, 32.f, 32.f);
    bounds.left += tx.getPosition().x;
    bounds.top += tx.getPosition().y;

    sf::FloatRect searchArea(tx.getPosition(), SeekSize);
    searchArea.left -= (SeekSize.x / 2.f);
    searchArea.top -= (SeekSize.y / 2.f);

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Filter::Solid);
    for (auto e : nearby)
    {
        //if (e != entity)
        {
            auto otherBounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
            auto otherPoint = sf::Vector2f(otherBounds.left + (otherBounds.width / 2.f), otherBounds.top + (otherBounds.height / 2.f));

            auto dir = otherPoint - tx.getPosition();

            sf::FloatRect overlap;
            if (bounds.intersects(otherBounds, overlap))
            {
                if (human.state == Human::State::Normal)
                {
                    //we've probably strayed from the path
                    human.state = Human::State::Seeking;
                }

                if (overlap.width < overlap.height)
                {
                    if (dir.x < 0)
                    {
                        tx.move(overlap.width, 0.f);
                        human.velocity = xy::Util::Vector::reflect(human.velocity, { 1.f, 0.f });
                    }
                    else
                    {
                        tx.move(-overlap.width, 0.f);
                        human.velocity = xy::Util::Vector::reflect(human.velocity, { -1.f, 0.f });
                    }
                }
                else
                {
                    if (dir.y < 0)
                    {
                        tx.move(0.f, overlap.height);
                        human.velocity = xy::Util::Vector::reflect(human.velocity, { 0.f, 1.f });
                    }
                    else
                    {
                        tx.move(0.f, -overlap.height);
                        human.velocity = xy::Util::Vector::reflect(human.velocity, { 0.f, -1.f });
                    }
                }
            }
        }
    }
}