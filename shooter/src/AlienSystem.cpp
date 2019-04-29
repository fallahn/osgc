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

namespace
{
    const sf::Time SpawnTime = sf::seconds(3.f);
    const std::size_t SpawnPerIndex = 40;// 4; //this many spawned at a point before moving to next spawn point
    const std::size_t MaxSpawns = 4;// 20;

    const sf::Vector2f FlockSize(256.f, 256.f);

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
        auto& alien = entity.getComponent<Alien>();

        alien.velocity += coalesce(entity);
        alien.velocity += separate(entity);
        alien.velocity += align(entity);

        auto& tx = entity.getComponent<xy::Transform>();
        tx.move(alien.velocity * dt);
        
        auto rotation = xy::Util::Vector::rotation(alien.velocity);
        tx.rotate(xy::Util::Math::shortestRotation(tx.getRotation(), rotation) * 50.f * dt);
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
}

//private
void AlienSystem::spawnAlien()
{
    //TODO find a better way of deciding which type of alien to breed
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
    entity.getComponent<Alien>().velocity = spawnOffsets[xy::Util::Random::value(0, 3)] * 20.f;

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionBox::Alien);
    entity.addComponent<CollisionBox>().type = CollisionBox::NPC;
    entity.addComponent<xy::SpriteAnimation>().play(1);

    LOG("Spawn radar icons for aliens", xy::Logger::Type::Info);
    m_debug = getScene()->createEntity();
    m_debug.addComponent<xy::Transform>();
    m_debug.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);

    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float)
    {
        if (!ConstVal::MapArea.contains(e.getComponent<xy::Transform>().getPosition()))
        {
            getScene()->destroyEntity(e);
        }
    };
}

sf::Vector2f AlienSystem::coalesce(xy::Entity entity)
{
    auto& tx = entity.getComponent<xy::Transform>();
    sf::FloatRect searchArea(tx.getPosition() - (FlockSize / 2.f), FlockSize);

    sf::Vector2f retVal;

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionBox::Alien);
    for (auto e : nearby)
    {
        //if (e != entity)
        {
            retVal += e.getComponent<xy::Transform>().getPosition();
        }
    }
    std::cout << nearby.size() << "\n";
    if (nearby.size() > 1)
    {
        retVal /= static_cast<float>(nearby.size() - 1);
    }

    return (retVal - tx.getPosition()) / 100.f;
}

sf::Vector2f AlienSystem::separate(xy::Entity entity)
{    
    static const float MinDistance = 64.f;
    static const float MinDistSqr = MinDistance * MinDistance;

    auto& tx = entity.getComponent<xy::Transform>();
    sf::FloatRect searchArea(tx.getPosition() - (FlockSize / 2.f), FlockSize);

    sf::Vector2f retVal;

    auto& verts = m_debug.getComponent<xy::Drawable>().getVertices();

    auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea);
    for (auto e : nearby)
    {
        if (e != entity)
        {
            auto otherBounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
            auto dir = getDistance(tx.getPosition(), otherBounds);

            if (xy::Util::Vector::lengthSquared(dir - tx.getPosition()) < MinDistSqr)
            {
                retVal -= (dir - tx.getPosition());
            }
            //TODO traditional collision handling with solid objects which overlap
            verts.emplace_back(sf::Vector2f(otherBounds.left, otherBounds.top), sf::Color::Magenta);
            verts.emplace_back(sf::Vector2f(otherBounds.left + otherBounds.width, otherBounds.top), sf::Color::Magenta);
            verts.emplace_back(sf::Vector2f(otherBounds.left + otherBounds.width, otherBounds.top + otherBounds.height), sf::Color::Magenta);
            verts.emplace_back(sf::Vector2f(otherBounds.left, otherBounds.top + otherBounds.height), sf::Color::Magenta);
            
            verts.emplace_back(sf::Vector2f(dir.x - 4.f, dir.y - 4.f), sf::Color::Cyan);
            verts.emplace_back(sf::Vector2f(dir.x + 4.f, dir.y - 4.f), sf::Color::Cyan);
            verts.emplace_back(sf::Vector2f(dir.x + 4.f, dir.y + 4.f), sf::Color::Cyan);
            verts.emplace_back(sf::Vector2f(dir.x - 4.f, dir.y + 4.f), sf::Color::Cyan);
        }
    }

    return retVal;
}

sf::Vector2f AlienSystem::align(xy::Entity entity)
{
    return {};
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