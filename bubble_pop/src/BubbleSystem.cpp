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
#include "MessageIDs.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <xyginext/util/Vector.hpp>

#ifdef XY_DEBUG
#include <xyginext/gui/Gui.hpp>
#endif

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

    std::array<int, 64u> debugGrid = { -1 };
}

BubbleSystem::BubbleSystem(xy::MessageBus& mb, NodeSet& ns)
    : xy::System(mb, typeid(BubbleSystem)),
    m_nodeSet   (ns)
{
    requireComponent<Bubble>();
    requireComponent<xy::Transform>();

#ifdef XY_DEBUG
    registerWindow([&]()
        {
            xy::Nim::begin("Grid");
            std::array<char, 9> str = { 
                char(debugGrid[0]),
                char(debugGrid[1]),
                char(debugGrid[2]),
                char(debugGrid[3]),
                char(debugGrid[4]),
                char(debugGrid[5]),
                char(debugGrid[6]),
                char(debugGrid[7]), 0};
            xy::Nim::text(str.data());

            str = { 
                char(debugGrid[8]),
                char(debugGrid[9]), 
                char(debugGrid[10]),
                char(debugGrid[11]),
                char(debugGrid[12]),
                char(debugGrid[13]),
                char(debugGrid[14]), 
                char(debugGrid[15]), 0 };
            xy::Nim::text(str.data());

            str = { 
                char(debugGrid[16]),
                char(debugGrid[17]),
                char(debugGrid[18]),
                char(debugGrid[19]),
                char(debugGrid[20]),
                char(debugGrid[21]),
                char(debugGrid[22]), 
                char(debugGrid[23]), 0 };
            xy::Nim::text(str.data());

            str = { 
                char(debugGrid[24]),
                char(debugGrid[25]),
                char(debugGrid[26]), 
                char(debugGrid[27]), 
                char(debugGrid[28]), 
                char(debugGrid[29]), 
                char(debugGrid[30]), 
                char(debugGrid[31]), 0 };
            xy::Nim::text(str.data());

            str = { 
                char(debugGrid[32]),
                char(debugGrid[33]),
                char(debugGrid[34]),
                char(debugGrid[35]),
                char(debugGrid[36]), 
                char(debugGrid[37]),
                char(debugGrid[38]),
                char(debugGrid[39]),
                0 };
            xy::Nim::text(str.data());

            str = { 
                char(debugGrid[40]),
                char(debugGrid[41]), 
                char(debugGrid[42]), 
                char(debugGrid[43]), 
                char(debugGrid[44]), 
                char(debugGrid[45]), 
                char(debugGrid[46]), 
                char(debugGrid[47]), 0 };
            xy::Nim::text(str.data());

            str = { 
                char(debugGrid[48]),
                char(debugGrid[49]), 
                char(debugGrid[50]), 
                char(debugGrid[51]), 
                char(debugGrid[52]), 
                char(debugGrid[53]), 
                char(debugGrid[54]), 
                char(debugGrid[55]), 0 };
            xy::Nim::text(str.data());

            str = { 
                char(debugGrid[56]),
                char(debugGrid[57]), 
                char(debugGrid[58]), 
                char(debugGrid[59]), 
                char(debugGrid[60]), 
                char(debugGrid[61]), 
                char(debugGrid[62]), 
                char(debugGrid[63]), 0 };
            xy::Nim::text(str.data());

            xy::Nim::end();
        });
#endif
}

//public
void BubbleSystem::process(float dt)
{
#ifdef XY_DEBUG   
    for (auto i = 0u; i < debugGrid.size(); ++i)
    {
        if (m_grid[i].isValid()) 
        {
            debugGrid[i] = m_grid[i].getComponent<Bubble>().colourType + 48;
        }
        else
        {
            debugGrid[i] = 45;
        }
    }
#endif

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
        }
            break;
        case Bubble::State::Suspended:
            //check for colour match
            if (bubble.testCluster)
            {
                //reset all states
                for (auto e : m_grid)
                {
                    if (e.isValid())
                    {
                        e.getComponent<Bubble>().processed = false;
                    }
                }

                if (auto cluster = fetchCluster(entity); cluster.size() > 2)
                {
                    //kill any bubbles if cluster >= 3
                    for (auto n : cluster)
                    {
                        //TODO raise a message
                        //TODO play death animation
                        m_grid[n].getComponent<Bubble>().state = Bubble::State::Dying;
                        m_grid[n] = {};
                    }

                    testFloating();
                }
                bubble.testCluster = false;
            }

            //if bubble in danger zone raise message
            if (entity.getComponent<xy::Transform>().getPosition().y + 
                m_nodeSet.barNode.getComponent<xy::Transform>().getPosition().y > Const::MaxBubbleHeight)
            {
                auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
                msg->type = GameEvent::RoundFailed;
                msg->generation = entity.getComponent<std::size_t>();
            }

            break;
        case Bubble::State::Dying:
            //these should already be removed from grid
            //data, so just destroy ent when anim is finished

            /*if (entity.getComponent<xy::SpriteAnimation>().stopped())
            {
                getScene()->destroyEntity(entity);
            }*/
            getScene()->destroyEntity(entity);
            break;
        }
    }
}

void BubbleSystem::resetGrid()
{
    for (auto& e : m_grid)
    {
        e = {};
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
    /*if (m_grid[gridIndex].isValid())
    {
        const auto& bubble = entity.getComponent<Bubble>();

        relPos += (bubble.velocity * -(Const::BubbleSize.x / 2.f));
        gridIndex = getGridIndex(relPos);

        if (m_grid[gridIndex].isValid())
        {
            xy::Logger::log("Still in occupied space!", xy::Logger::Type::Warning);
        }

        collides = true;
    }*/

    if (!collides) //check surrounding
    {
        //calc if even or odd row (affects which cells to test)
        std::vector<std::int32_t> gridIndices = getNeighbours(gridIndex);        

        //test surrounding cells, if we collide change state and snap to grid pos
        //and parent to top bar
        for (auto i : gridIndices)
        {
            if (i > -1 && i < m_grid.size())
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
        entity.getComponent<Bubble>().gridIndex = gridIndex;
        entity.getComponent<Bubble>().testCluster = true; //we want to test for colour match next pass.
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

std::vector<std::int32_t> BubbleSystem::fetchCluster(xy::Entity entity, bool matchAll)
{    
    std::vector<std::int32_t> testList;
    testList.push_back(entity.getComponent<Bubble>().gridIndex);

    entity.getComponent<Bubble>().processed = true;
    auto targetColour = matchAll ? -1 : entity.getComponent<Bubble>().colourType;

    std::vector<std::int32_t> clusterList; //results

    while (!testList.empty())
    {
        auto currentIndex = testList.back();
        testList.pop_back();

        if (!m_grid[currentIndex].isValid())
        {
            continue;
        }

        if (m_grid[currentIndex].getComponent<Bubble>().colourType == targetColour
            || matchAll) //match all used when looking for floating clusters
        {
            clusterList.push_back(currentIndex);

            auto neighbours = getNeighbours(currentIndex);
            for (auto n : neighbours)
            {
                if ((n > -1 && n < m_grid.size()) &&
                    m_grid[n].isValid() &&
                    !m_grid[n].getComponent<Bubble>().processed)
                {
                    testList.push_back(n);
                    m_grid[n].getComponent<Bubble>().processed = true;
                }
            }
        }
    }

    return clusterList;
}

void BubbleSystem::testFloating()
{
    //reset all states
    for (auto e : m_grid)
    {
        if (e.isValid())
        {
            e.getComponent<Bubble>().processed = false;
        }
    }

    //clusters to test
    std::vector<std::vector<std::int32_t>> clusters;

    for (auto i = 0; i < m_grid.size(); ++i)
    {
        if (m_grid[i].isValid()
            && !m_grid[i].getComponent<Bubble>().processed)
        {
            auto cluster = fetchCluster(m_grid[i], true);
            bool floating = true;

            for (auto j : cluster)
            {
                if (j < Const::BubblesPerRow)
                {
                    //on the top row therefore still attached
                    floating = false;
                    break;
                }
            }

            if (floating)
            {
                clusters.push_back(cluster);
            }
        }
    }

    //remove all bubbles in found clusters
    for (auto& c : clusters)
    {
        for (auto b : c)
        {
            m_grid[b].getComponent<Bubble>().state = Bubble::State::Dying;
            m_grid[b] = {};
        }
    }
}

std::vector<std::int32_t> BubbleSystem::getNeighbours(std::int32_t gridIndex) const
{
    std::vector<std::int32_t> gridIndices;

    gridIndices.push_back(gridIndex - 1);
    gridIndices.push_back(gridIndex + 1);
    gridIndices.push_back(gridIndex - Const::BubblesPerRow);
    gridIndices.push_back(gridIndex + Const::BubblesPerRow);

    if ((gridIndex / Const::BubblesPerRow) % 2 != 0)
    {
        //offset to left
        if (gridIndices[2] % Const::BubblesPerRow != 0)
        {
            gridIndices.push_back(gridIndices[2] - 1);
        }

        if (gridIndices[3] % Const::BubblesPerRow != 0)
        {
            gridIndices.push_back(gridIndices[3] - 1);
        }
    }
    else
    {
        //offset to right
        if (gridIndices[2] % Const::BubblesPerRow != (Const::BubblesPerRow - 1))
        {
            gridIndices.push_back(gridIndices[2] + 1);
        }

        if (gridIndices[3] % Const::BubblesPerRow != (Const::BubblesPerRow - 1))
        {
            gridIndices.push_back(gridIndices[3] + 1);
        }
    }
    return gridIndices;
}

void BubbleSystem::onEntityAdded(xy::Entity entity)
{
    const auto& bubble = entity.getComponent<Bubble>();
    if (bubble.gridIndex > -1)
    {
        m_grid[bubble.gridIndex] = entity;
    }
}

void BubbleSystem::onEntityRemoved(xy::Entity entity)
{
    if (getEntities().size() == 3)
    {
        //only queued and mounted remain
        auto* msg = postMessage<GameEvent>(MessageID::GameMessage);
        msg->type = GameEvent::RoundCleared;
        msg->generation = entity.getComponent<std::size_t>();
    }
}