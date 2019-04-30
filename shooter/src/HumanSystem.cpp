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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

namespace
{
    const sf::Vector2f SeekSize(512.f, 512.f);

    const sf::Time SpawnTime = sf::seconds(2.8f);
    const std::size_t SpawnsPerPoint = 4;
    const std::size_t MaxSpawns = 24;

    const float NodeDistanceSqr = (Node::Bounds[2] / 2.f) * (Node::Bounds[2] / 2.f);
}

HumanSystem::HumanSystem(xy::MessageBus& mb, const SpriteArray& sprites)
    : xy::System(mb, typeid(HumanSystem)),
    m_sprites   (sprites),
    m_spawnIndex(0),
    m_spawnCount(0)
{
    requireComponent<xy::Transform>();
    requireComponent<Human>();
    requireComponent<Navigator>();
}

//public
void HumanSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        const auto& human = entity.getComponent<Human>();
        switch (human.state)
        {
        default: break;
        case Human::State::Normal:
            updateNormal(entity, dt);
            break;
        case Human::State::Seeking:
            updateSeeking(entity, dt);
            break;
        }
    }

    //check if OK to spawn new human
    if (entities.size() < MaxSpawns)
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

void HumanSystem::clearSpawns()
{
    m_spawnPoints.clear();
    m_spawnIndex = 0;
    m_spawnCount = 0;
}

//private
void HumanSystem::spawnHuman()
{
    auto entity = getScene()->createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_spawnPoints[m_spawnIndex]);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 2);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Human];
    entity.addComponent<xy::SpriteAnimation>().play(1);
    auto bounds = m_sprites[SpriteID::Human].getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
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

    //apparently there's no radar icon for humans...
    /*auto humanEntity = entity;
    entity = getScene()->createEntity();
    entity.addComponent<xy::Transform>().setPosition(-100.f, -100.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
    entity.addComponent<xy::Sprite>() = m_sprites[];*/
}

void HumanSystem::updateNormal(xy::Entity entity, float dt)
{
    //follow velocity until at new node
    //TODO probably move this to beginning of process() func
    auto& tx = entity.getComponent<xy::Transform>();
    auto& human = entity.getComponent<Human>();
    tx.move(human.velocity * human.speed * dt);
    tx.rotate(xy::Util::Math::shortestRotation(tx.getRotation(), xy::Util::Vector::rotation(human.velocity)));

    const auto& nav = entity.getComponent<Navigator>();
    auto dir = tx.getPosition() - nav.target.getComponent<xy::Transform>().getPosition();
    if (xy::Util::Vector::lengthSquared(dir) < NodeDistanceSqr)
    {
        human.state = Human::State::Seeking;
    }


    //if wall collision then switch to seeking for new node?
    //needs to prevent seeking through solid objects... see line intersection func in alien system
}

void HumanSystem::updateSeeking(xy::Entity entity, float dt)
{
    //walk randomly while bouncing off walls until finding a new node target
    auto& tx = entity.getComponent<xy::Transform>();
    auto& human = entity.getComponent<Human>();
    tx.move(human.velocity * human.speed * dt);
    tx.rotate(xy::Util::Math::shortestRotation(tx.getRotation(), xy::Util::Vector::rotation(human.velocity)));

    auto bounds = tx.getTransform().transformRect(entity.getComponent<xy::BroadphaseComponent>().getArea());
    sf::FloatRect searchArea(tx.getPosition(), SeekSize);
    searchArea.left -= (SeekSize.x / 2.f);
    searchArea.top -= (SeekSize.y / 2.f);

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Filter::Solid | CollisionBox::Filter::Navigation);
    for(auto e : nearby)
    {       
        if (e != entity)
        {
            if (e.getComponent<xy::BroadphaseComponent>().getFilterFlags() & CollisionBox::Navigation)
            {
                //TODO cast ray to found node and check for solid collision
                //TODO collect a list of candidates and choose which has the nearest direction to current velocity

                //make sure this wasn't the previous node
                auto& nav = entity.getComponent<Navigator>();
                const auto& node = e.getComponent<Node>();

                if (nav.previousNode != node.ID)
                {
                    if (nav.target.isValid())
                    {
                        nav.previousNode = nav.target.getComponent<Node>().ID;
                    }
                    nav.target = e;

                    human.velocity = e.getComponent<xy::Transform>().getPosition() - tx.getPosition();
                    human.velocity = xy::Util::Vector::normalise(human.velocity);

                    human.state = Human::State::Normal;
                    return;
                }
            }
            else
            {
                //TODO this probably wants to be done in main process() func after all updates complete
                auto otherBounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
                auto otherPoint = sf::Vector2f(otherBounds.left + (otherBounds.width / 2.f), otherBounds.top + (otherBounds.height / 2.f));

                auto dir = otherPoint - tx.getPosition();

                sf::FloatRect overlap;
                if (bounds.intersects(otherBounds, overlap))
                {
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
}