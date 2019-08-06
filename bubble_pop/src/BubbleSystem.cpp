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

#include "BubbleSystem.hpp"
#include "NodeSet.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>

BubbleSystem::BubbleSystem(xy::MessageBus& mb, NodeSet& ns)
    : xy::System(mb, typeid(BubbleSystem)),
    m_nodeSet   (ns)
{
    requireComponent<Bubble>();
    requireComponent<xy::Transform>();
}

//public
void BubbleSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& bubble = entity.getComponent<Bubble>();
        switch (bubble.state)
        {
        default: break;
        case Bubble::State::Queuing:
        {
            auto& tx = entity.getComponent<xy::Transform>();
            tx.move(Bubble::Speed * dt, 0.f);

            //check position of gun mount as it may be moving
            auto gunPos = m_nodeSet.gunNode.getComponent<xy::Transform>().getPosition();
            if (std::abs(gunPos.x - tx.getPosition().x) < 10.f)
            {
                m_nodeSet.rootNode.getComponent<xy::Transform>().removeChild(tx);
                m_nodeSet.gunNode.getComponent<xy::Transform>().addChild(tx);
                tx.setPosition(m_nodeSet.gunNode.getComponent<xy::Transform>().getOrigin());
                bubble.state = Bubble::State::Mounted;
            }
        }
            break;
        case Bubble::State::Firing:
        {
            auto& tx = entity.getComponent<xy::Transform>();
            tx.move(bubble.velocity * Bubble::Speed * dt);

            //TEMP
            sf::FloatRect area(0.f, 0.f, 1280.f, 960.f);
            if (!area.contains(tx.getPosition()))
            {
                getScene()->destroyEntity(entity);
            }
        }
            break;
        }
    }
}