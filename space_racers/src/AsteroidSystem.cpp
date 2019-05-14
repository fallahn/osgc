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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Vector.hpp>

AsteroidSystem::AsteroidSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(AsteroidSystem))
{
    requireComponent<Asteroid>();
    requireComponent<xy::Transform>();
}

//public
void AsteroidSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& tx = entity.getComponent<xy::Transform>();
        auto& roid = entity.getComponent<Asteroid>();

        //update position
        tx.move(roid.getVelocity() * roid.getSpeed() * dt);
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
            tx.move(position.x - (m_mapSize.left + m_mapSize.width), 0.f);
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), sf::Vector2f(-1.f, 0.f)));
        }

        if (position.y < m_mapSize.top)
        {
            tx.move(0.f, m_mapSize.top - position.y);
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), sf::Vector2f(0.f, 1.f)));
        }
        else if (position.y > m_mapSize.top + m_mapSize.height)
        {
            tx.move(0.f, position.y - (m_mapSize.top + m_mapSize.height));
            roid.setVelocity(xy::Util::Vector::reflect(roid.getVelocity(), sf::Vector2f(0.f, -1.f)));
        }


        //TODO collision detection
    }
}