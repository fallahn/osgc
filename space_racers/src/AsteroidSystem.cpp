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

#include "AsteroidSystem.hpp"
#include "CollisionObject.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    //area around a roid to search for collisions
    const sf::FloatRect SearchArea(0.f, 0.f, 200.f, 200.f);
}

AsteroidSystem::AsteroidSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(AsteroidSystem))
{
    requireComponent<Asteroid>();
    requireComponent<xy::Transform>();
    requireComponent<xy::BroadphaseComponent>();
}

//public
void AsteroidSystem::process(float dt)
{
    m_collisionPairs.clear();

    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto& roid = entity.getComponent<Asteroid>();

        //update position
        tx.move(roid.getVelocity() /** roid.getSpeed()*/ * dt);
        auto position = tx.getPosition();

        //just do simple reflection to keep roids in the map area
        //teleporting can do funny things to the physics...
        if (position.x < m_mapSize.left)
        {
            tx.move(m_mapSize.left - position.x, 0.f);
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), sf::Vector2f(1.f, 0.f)));
        }
        else if (position.x > m_mapSize.left + m_mapSize.width)
        {
            tx.move((m_mapSize.left + m_mapSize.width) - position.x, 0.f);
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), sf::Vector2f(-1.f, 0.f)));
        }

        if (position.y < m_mapSize.top)
        {
            tx.move(0.f, m_mapSize.top - position.y);
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), sf::Vector2f(0.f, 1.f)));
        }
        else if (position.y > m_mapSize.top + m_mapSize.height)
        {
            tx.move(0.f, (m_mapSize.top + m_mapSize.height) - position.y);
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), sf::Vector2f(0.f, -1.f)));
        }


        //collision detection broadphase
        auto searchArea = SearchArea;
        searchArea.left += position.x;
        searchArea.top += position.y;

        auto aabb = entity.getComponent<xy::BroadphaseComponent>().getArea();
        aabb = tx.getTransform().transformRect(aabb);

        auto nearby = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, CollisionFlags::Asteroid);
        for (auto other : nearby)
        {
            if (other != entity)
            {
                auto otherAabb = other.getComponent<xy::BroadphaseComponent>().getArea();
                otherAabb = other.getComponent<xy::Transform>().getTransform().transformRect(otherAabb);

                if (otherAabb.intersects(aabb))
                {
                    m_collisionPairs.insert(std::minmax(other, entity));
                }
            }
        }
    }

    //collision narrow phase
    for (auto& [a, b] : m_collisionPairs)
    {
        //copy these because a and b are const...
        auto entA = a;
        auto entB = b;

        auto& txA = entA.getComponent<xy::Transform>();
        auto& txB = entB.getComponent<xy::Transform>();

        auto& roidA = entA.getComponent<Asteroid>();
        auto& roidB = entB.getComponent<Asteroid>();

        auto diff = txA.getPosition() - txB.getPosition();
        auto dist = roidA.getRadius() + roidB.getRadius();
        auto dist2 = dist * dist;

        //check for overlap
        auto len2 = xy::Util::Vector::lengthSquared(diff);
        if (len2 < dist2)
        {
            //separate if overlapping
            auto len = std::sqrt(len2);
            auto penetration = dist - len;
            auto normal = diff / len;

            txA.move(normal * (penetration));
            txB.move(-normal * (penetration));

            //calc new vectors using each mass to update momentum
            float vA = xy::Util::Vector::dot(roidA.getVelocity(), normal);
            float vB = xy::Util::Vector::dot(roidB.getVelocity(), normal);
            float momentum = (2.f * (vA - vB)) / (roidA.getMass() + roidB.getMass());

            sf::Vector2f velA = roidA.getVelocity() - (normal * (momentum * roidB.getMass()));
            sf::Vector2f velB = roidB.getVelocity() + (normal * (momentum * roidA.getMass()));

            roidA.setVelocity(velA);
            roidB.setVelocity(velB);
        }
    }
}