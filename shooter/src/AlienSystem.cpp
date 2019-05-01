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

#include "Alien.hpp"
#include "GameConsts.hpp"
#include "CollisionTypes.hpp"
#include "RadarItemSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>

#include <algorithm>

namespace
{
    const sf::Time SpawnTime = sf::seconds(3.f);
    const std::size_t SpawnPerIndex = 5; //this many spawned at a point before moving to next spawn point
    const std::size_t MaxSpawns = 10;
    const float MaxVelocity = 215.f;
    const float MaxVelocitySqr = MaxVelocity * MaxVelocity;

    const float MinDistance = 76.f;
    const float MinDistSqr = MinDistance * MinDistance;

    const float MaxCoalescenceRadius = 96.f * 96.f;

    const sf::Vector2f FlockSize(360.f, 360.f);

    bool intersects(const std::pair<sf::Vector2f, sf::Vector2f>& segOne,
        const std::pair<sf::Vector2f, sf::Vector2f>& segTwo, sf::Vector2f& intersection)
    {
        sf::Vector2f dirOne = segOne.second - segOne.first;
        sf::Vector2f dirTwo = segTwo.second - segTwo.first;

        float s = (-dirOne.y * (segOne.first.x - segTwo.first.x) + dirOne.x * (segOne.first.y - segTwo.first.y)) / (-dirTwo.x * dirOne.y + dirOne.x * dirTwo.y);
        float t = (dirTwo.x * (segOne.first.y - segTwo.first.y) - dirTwo.y * (segOne.first.x - segTwo.first.x)) / (-dirTwo.x * dirOne.y + dirOne.x * dirTwo.y);

        if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
        {
            //collision detected
            intersection.x = segOne.first.x + (t * dirOne.x);
            intersection.y = segOne.first.y + (t * dirOne.y);
            return true;
        }

        return false;
    }
}

AlienSystem::AlienSystem(xy::MessageBus& mb, const SpriteArray& sprites)
    : xy::System(mb, typeid(AlienSystem)),
    m_sprites   (sprites),
    m_spawnIndex(0),
    m_spawnCount(0)
{
    requireComponent<xy::Transform>();
    requireComponent<Alien>();
}

//public
void AlienSystem::process(float dt)
{
    if (m_debug.isValid())
    {
        m_debug.getComponent<xy::Drawable>().getVertices().clear();
    }

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        Results results;
        processBoid(entity, results);
        
        auto& alien = entity.getComponent<Alien>();
        alien.velocity += results.separation;
        alien.velocity += results.alignment;
        alien.velocity += results.coalescence;
        alien.velocity += results.humanAttraction;

        //limit velocity
        auto len2 = xy::Util::Vector::lengthSquared(alien.velocity);
        if (len2 > MaxVelocitySqr)
        {
            alien.velocity /= std::sqrt(len2);
            alien.velocity *= MaxVelocity;
        }

        auto& tx = entity.getComponent<xy::Transform>();
        tx.move(alien.velocity * dt);
        
        solidCollision(entity);

        auto rotation = xy::Util::Vector::rotation(alien.velocity);
        tx.rotate(xy::Util::Math::shortestRotation(tx.getRotation(), rotation) * 7.f * dt);
    }

    if (entities.size() < MaxSpawns)
    {
        if (m_spawnClock.getElapsedTime() > SpawnTime)
        {
            m_spawnClock.restart();
            spawnAlien();

            m_spawnCount = (m_spawnCount + 1) % SpawnPerIndex;
            if (m_spawnCount == 0)
            {
                //move to next spawn point
                m_spawnIndex = (m_spawnIndex + 1) % m_spawnPoints.size();
            }
        }
    }
    else
    {
        m_spawnClock.restart();
    }

    if (m_debug.isValid())
    {
        m_debug.getComponent<xy::Drawable>().updateLocalBounds();
    }
}

void AlienSystem::addSpawn(sf::Vector2f position)
{
    m_spawnPoints.push_back(position);
    std::shuffle(m_spawnPoints.begin(), m_spawnPoints.end(), xy::Util::Random::rndEngine);
}

void AlienSystem::clearSpawns()
{
    m_spawnPoints.clear();
    m_spawnCount = 0;
    m_spawnIndex = 0;
}

//private
void AlienSystem::spawnAlien()
{
    //TODO find a better way of deciding which type of alien to breed
    //particularly as beetles are more valuable
    auto alienType = xy::Util::Random::value(0, 1);

    static const std::array<sf::Vector2f, 4u> spawnOffsets =
    {
        sf::Vector2f(-5.f, -5.f),
        sf::Vector2f(5.f, -5.f),
        sf::Vector2f(5.f, 5.f),
        sf::Vector2f(-5.f, 5.f),
    };

    auto entity = getScene()->createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_spawnPoints[m_spawnIndex] + spawnOffsets[xy::Util::Random::value(0, 3)]);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 2);
    entity.addComponent<xy::Sprite>() = alienType == 0 ? m_sprites[SpriteID::Beetle] : m_sprites[SpriteID::Scorpion];
    entity.addComponent<Alien>().type = alienType == 0 ? Alien::Type::Beetle : Alien::Type::Scorpion;
    entity.getComponent<Alien>().velocity = spawnOffsets[xy::Util::Random::value(0, 3)] * 10.f;

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionBox::Alien);
    entity.addComponent<CollisionBox>().type = CollisionBox::NPC;
    entity.getComponent<CollisionBox>().height = 12.f;
    entity.addComponent<xy::SpriteAnimation>().play(1);

    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float)
    {
        if (!ConstVal::MapArea.contains(e.getComponent<xy::Transform>().getPosition()))
        {
            getScene()->destroyEntity(e);
        }
    };
    auto alienEntity = entity;
    
    //radar version
    entity = getScene()->createEntity();
    entity.addComponent<xy::Transform>().setPosition(-100.f, -100.f);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
    entity.addComponent<xy::Sprite>() = alienType == 0 ? m_sprites[SpriteID::BeetleIcon] : m_sprites[SpriteID::ScorpionIcon];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);
    entity.addComponent<RadarItem>().parent = alienEntity;

    //FUTURE ME: we only want one of these per scene, not per alien
    //m_debug = getScene()->createEntity();
    //m_debug.addComponent<xy::Transform>();
    //m_debug.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
}

void AlienSystem::processBoid(xy::Entity entity, Results& results)
{
    auto& tx = entity.getComponent<xy::Transform>();
    sf::FloatRect searchArea(tx.getPosition() - (FlockSize / 2.f), FlockSize);

    sf::Vector2f centreOfMass;
    sf::Vector2f avgVelocity;
    float avgCount = 0;

    sf::Vector2f humanTarget;
    float humanCount = 0.f;

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Alien | /*CollisionBox::Filter::Solid |*/ CollisionBox::Human);
    for (auto e : nearby)
    {
        if (e != entity)
        {
            auto otherPosition = e.getComponent<xy::Transform>().getPosition();
            const auto& otherBroadphase = e.getComponent<xy::BroadphaseComponent>();

            if (otherBroadphase.getFilterFlags() & CollisionBox::Filter::Alien)
            {
                centreOfMass += otherPosition;
                avgVelocity += e.getComponent<Alien>().velocity;
                avgCount++;
            }
            else if (otherBroadphase.getFilterFlags() & CollisionBox::Filter::Human)
            {
                humanTarget += otherPosition;
                humanCount++;
            }
            else
            {
                auto bounds = otherBroadphase.getArea();
                bounds = e.getComponent<xy::Transform>().getTransform().transformRect(bounds);
                otherPosition = { bounds.left + (bounds.width / 2.f), bounds.top + (bounds.height / 2.f) };
            }

            auto dir = otherPosition - tx.getPosition();
            if (xy::Util::Vector::lengthSquared(dir) < MinDistSqr)
            {
                results.separation -= dir;
            }
        }
    }

    if(avgCount > 0)
    {
        //if (nearby.size() > 1)
        {
            centreOfMass /= avgCount;
            avgVelocity /= avgCount;
        }
        auto& alien = entity.getComponent<Alien>();
        //DPRINT("com", std::to_string(centreOfMass.x) + ", " + std::to_string(centreOfMass.y));
        auto coalescenceDirection = centreOfMass - tx.getPosition();
        auto coalescenceStrength = /*1.f - */xy::Util::Math::clamp(xy::Util::Vector::lengthSquared(coalescenceDirection) / MaxCoalescenceRadius, 0.f, 1.f);
        results.coalescence = (coalescenceDirection/* * coalescenceStrength*/) / 100.f;
        //DPRINT("align", std::to_string(avgVelocity.x) + ", " + std::to_string(avgVelocity.y));
        results.alignment = ((avgVelocity - alien.velocity) / 16.f);
    }

    if (humanCount > 0)
    {
        humanTarget /= humanCount;
        results.humanAttraction = ((humanTarget - tx.getPosition()) / 4.f);
    }
}

void AlienSystem::solidCollision(xy::Entity entity)
{    
    auto& tx = entity.getComponent<xy::Transform>();
    auto& alien = entity.getComponent<Alien>();

    sf::FloatRect searchArea(tx.getPosition() - (FlockSize / 2.f), FlockSize);
    sf::FloatRect boidBounds(-16.f, -16.f, 32.f, 32.f);// = entity.getComponent<xy::BroadphaseComponent>().getArea();
    //we want to skip rotation here which might grow the AABB
    boidBounds.left += tx.getPosition().x;// -(boidBounds.width * 2.f);
    boidBounds.top += tx.getPosition().y;// -(boidBounds.height * 2.f);
    //boidBounds.width *= tx.getScale().x;
   // boidBounds.height *= tx.getScale().y;
    //boidBounds = tx.getTransform().transformRect(boidBounds);

    /*auto& verts = m_debug.getComponent<xy::Drawable>().getVertices();
    verts.emplace_back(sf::Vector2f(boidBounds.left, boidBounds.top), sf::Color::Green);
    verts.emplace_back(sf::Vector2f(boidBounds.left + boidBounds.width, boidBounds.top), sf::Color::Green);
    verts.emplace_back(sf::Vector2f(boidBounds.left + boidBounds.width, boidBounds.top + boidBounds.height), sf::Color::Green);
    verts.emplace_back(sf::Vector2f(boidBounds.left, boidBounds.top + boidBounds.height), sf::Color::Green);*/

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Solid);
    for (auto e : nearby)
    {
        //if (e != entity)
        {
            auto otherBounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
            auto otherPoint = sf::Vector2f(otherBounds.left + (otherBounds.width / 2.f), otherBounds.top + (otherBounds.height / 2.f));

            auto dir = otherPoint - tx.getPosition();

            sf::FloatRect overlap;
            if (boidBounds.intersects(otherBounds, overlap))
            {
                if (overlap.width < overlap.height)
                {
                    if (dir.x < 0)
                    {
                        tx.move(overlap.width, 0.f);
                        alien.velocity = xy::Util::Vector::reflect(alien.velocity, { 1.f, 0.f });
                    }
                    else
                    {
                        tx.move(-overlap.width, 0.f);
                        alien.velocity = xy::Util::Vector::reflect(alien.velocity, { -1.f, 0.f });
                    }
                }
                else
                {
                    if (dir.y < 0)
                    {
                        tx.move(0.f, overlap.height);
                        alien.velocity = xy::Util::Vector::reflect(alien.velocity, { 0.f, 1.f });
                    }
                    else
                    {
                        tx.move(0.f, -overlap.height);
                        alien.velocity = xy::Util::Vector::reflect(alien.velocity, { 0.f, -1.f });
                    }
                }
            }


            //debug draw
            /*verts.emplace_back(sf::Vector2f(otherBounds.left, otherBounds.top), sf::Color::Magenta);
            verts.emplace_back(sf::Vector2f(otherBounds.left + otherBounds.width, otherBounds.top), sf::Color::Magenta);
            verts.emplace_back(sf::Vector2f(otherBounds.left + otherBounds.width, otherBounds.top + otherBounds.height), sf::Color::Magenta);
            verts.emplace_back(sf::Vector2f(otherBounds.left, otherBounds.top + otherBounds.height), sf::Color::Magenta);
            
            verts.emplace_back(sf::Vector2f(otherPoint.x - 4.f, otherPoint.y - 4.f), sf::Color::Cyan);
            verts.emplace_back(sf::Vector2f(otherPoint.x + 4.f, otherPoint.y - 4.f), sf::Color::Cyan);
            verts.emplace_back(sf::Vector2f(otherPoint.x + 4.f, otherPoint.y + 4.f), sf::Color::Cyan);
            verts.emplace_back(sf::Vector2f(otherPoint.x - 4.f, otherPoint.y + 4.f), sf::Color::Cyan);*/
        }
    }
}

sf::Vector2f AlienSystem::getDistance(sf::Vector2f point, sf::FloatRect target)
{
    sf::Vector2f targetCentre(target.left + (target.width / 2.f), target.top + (target.height / 2.f));

    std::pair<sf::Vector2f, sf::Vector2f> firstSegment = std::make_pair(point, targetCentre);

    sf::Vector2f retVal;
    if (targetCentre.x < point.x && targetCentre.y < point.y)
    {
        //up left
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment = 
            std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left + target.width, target.top + target.height));

        if (!intersects(firstSegment, secondSegment, retVal))
        {
            secondSegment = 
                std::make_pair(sf::Vector2f(target.left + target.width, target.top + target.height), sf::Vector2f(target.left + target.width, target.top));
            intersects(firstSegment, secondSegment, retVal);
        }
    }
    else if (targetCentre.x > point.x && targetCentre.y < point.y)
    {
        //up right
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment =
            std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left + target.width, target.top + target.height));

        if (!intersects(firstSegment, secondSegment, retVal))
        {
            secondSegment = 
                std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left, target.top));
            intersects(firstSegment, secondSegment, retVal);
        }
    }
    else if (targetCentre.x > point.x && targetCentre.y > point.y)
    {
        //down right
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment =
            std::make_pair(sf::Vector2f(target.left, target.top), sf::Vector2f(target.left + target.width, target.top));

        if (!intersects(firstSegment, secondSegment, retVal))
        {
            secondSegment = 
                std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left, target.top));
            intersects(firstSegment, secondSegment, retVal);
        }
    }
    else if (targetCentre.x < point.x && targetCentre.y > point.y)
    {
        //down left
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment =
            std::make_pair(sf::Vector2f(target.left, target.top), sf::Vector2f(target.left + target.width, target.top));

        if (!intersects(firstSegment, secondSegment, retVal))
        {
            secondSegment = 
                std::make_pair(sf::Vector2f(target.left + target.width, target.top + target.height), sf::Vector2f(target.left + target.width, target.top));
            intersects(firstSegment, secondSegment, retVal);
        }
    }

    return retVal;// -point;
};