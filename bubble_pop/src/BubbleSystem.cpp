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
#include "GameConsts.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    //remember the input to this needs to be relative to the top bar node position
    std::int32_t getGridIndex(sf::Vector2f position)
    {
        std::int32_t yPos = static_cast<std::int32_t>(std::floor(position.y / Const::RowHeight));

        //check which row we're on and offset x pos back to square grid
        if (yPos % 2 == 0)
        {
            position.x -= (Const::BubbleSize.x / 2.f);
        }

        std::int32_t xPos = static_cast<std::int32_t>(std::floor(position.x / Const::BubbleSize.x));

        return (yPos * Const::BubblesPerRow) + xPos;
    }
}

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

            doCollision(entity);

            //TEMP
            /*sf::FloatRect area(0.f, 0.f, 1280.f, 960.f);
            if (!area.contains(tx.getPosition()))
            {
                getScene()->destroyEntity(entity);
            }*/
        }
            break;
        case Bubble::State::Suspended:
            //TODO check for colour match
            break;
        }
    }
}

//private
void BubbleSystem::doCollision(xy::Entity entity)
{
    auto& tx = entity.getComponent<xy::Transform>();
    auto position = tx.getPosition();

    //check against bounds
    if (position.x < Const::LeftBounds)
    {
        position.x = Const::LeftBounds;
        tx.setPosition(position);
        entity.getComponent<Bubble>().velocity = xy::Util::Vector::reflect(entity.getComponent<Bubble>().velocity, { 1.f, 0.f });
    }
    else if (position.x > Const::RightBounds)
    {
        position.x = Const::RightBounds;
        tx.setPosition(position);

        entity.getComponent<Bubble>().velocity = xy::Util::Vector::reflect(entity.getComponent<Bubble>().velocity, { -1.f, 0.f });
    }


    //get current grid index
    auto relPos = position - m_nodeSet.barNode.getComponent<xy::Transform>().getPosition();
    auto gridIndex = getGridIndex(relPos);

    bool collides = false;
    //if we tunnelled collision, move back one radius
    if (m_grid[gridIndex].isValid())
    {
        const auto& bubble = entity.getComponent<Bubble>();

        relPos += (bubble.velocity * -(Const::BubbleSize.x / 2.f));
        gridIndex = getGridIndex(relPos);

        if (m_grid[gridIndex].isValid())
        {
            xy::Logger::log("Still in occupied space!", xy::Logger::Type::Warning);
        }

        collides = true;
    }

    if (!collides) //check surrounding
    {
        //calc if even or odd row (affects which cells to test)
        std::vector<std::int32_t> gridIndices;
        gridIndices.push_back(gridIndex - 1);
        gridIndices.push_back(gridIndex + 1);
        gridIndices.push_back(gridIndex - Const::BubblesPerRow);
        gridIndices.push_back(gridIndex + Const::BubblesPerRow);

        if (gridIndex % 2 == 0)
        {
            //offset to right
            gridIndices.push_back((gridIndex - Const::BubblesPerRow) + 1);
            gridIndices.push_back((gridIndex + Const::BubblesPerRow) + 1);
        }
        else
        {
            //offset to left
            gridIndices.push_back((gridIndex - Const::BubblesPerRow) - 1);
            gridIndices.push_back((gridIndex + Const::BubblesPerRow) - 1);
        }

        //test surrounding cells, if we collide change state and snap to grid pos
        //and parent to top bar
        for (auto i : gridIndices)
        {
            if (i > -1 && i < Const::GridSize)
            {
                if (m_grid[i].isValid())
                {
                    auto len2 = xy::Util::Vector::lengthSquared(relPos- m_grid[i].getComponent<xy::Transform>().getPosition());
                    if (len2 < Const::BubbleDistSqr)
                    {
                        collides = true;
                        break;
                    }
                }
            }
        }
    }

    auto attachBubble = [&]()
    {
        auto pos = tileToWorldCoord(gridIndex) + tx.getOrigin();
        tx.setPosition(pos);
        m_nodeSet.rootNode.getComponent<xy::Transform>().removeChild(tx);
        m_nodeSet.barNode.getComponent<xy::Transform>().addChild(tx);
        entity.getComponent<Bubble>().state = Bubble::State::Suspended;
        m_grid[gridIndex] = entity;
    };
    
    if (collides)
    {
        attachBubble();
    }
    else
    {
        //if no collision check top bar collision and snap to grid pos if there is
        auto topBounds = (Const::BubbleSize.y / 2.f) + m_nodeSet.barNode.getComponent<xy::Transform>().getPosition().y;
        if (position.y < topBounds)
        {
            attachBubble();
        }
    }
}

void BubbleSystem::onEntityAdded(xy::Entity entity)
{
    const auto& bubble = entity.getComponent<Bubble>();
    if (bubble.gridIndex > -1)
    {
        m_grid[bubble.gridIndex] = entity;
    }
}