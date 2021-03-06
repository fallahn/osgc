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

#include "PathFinder.hpp"

#include <xyginext/core/Assert.hpp>

#include <SFML/System/Lock.hpp>
#include <SFML/System/Sleep.hpp>

#include <array>
#include <iostream>
#include <algorithm>

namespace
{
    const std::array<sf::Vector2i, 8> directions = 
    {
        sf::Vector2i(0, 1),
        sf::Vector2i(1, 0),
        sf::Vector2i(0, -1),
        sf::Vector2i(-1, 0),
        sf::Vector2i(-1, -1),
        sf::Vector2i(1, 1),
        sf::Vector2i(-1, 1),
        sf::Vector2i(1, -1)
    };

    //manhatten
    const sf::Uint8 D = 1;// 0;
    const sf::Uint8 D2 = D;
    sf::Uint8 heuristic(const sf::Vector2i& start, const sf::Vector2i& end)
    {
        sf::Vector2i delta(std::abs(start.x - end.x), std::abs(start.y - end.y));
        return D * delta.x + delta.y;
    }

    //diagonal
    sf::Uint8 heuristicDiagonal(const sf::Vector2i& start, const sf::Vector2i& end)
    {
        /*dx = abs(node.x - goal.x)
            dy = abs(node.y - goal.y)
            return D * (dx + dy) + (D2 - 2 * D) * min(dx, dy)*/

        sf::Vector2i delta(std::abs(start.x - end.x), std::abs(start.y - end.y));
        return D * (delta.x + delta.y) + (D2 - 2 * D) * std::min(delta.x, delta.y);
    }
}

PathFinder::PathFinder()
    : m_thread(&PathFinder::processQueue, this),
    m_threadRunning(true)
{
    m_thread.launch();
}

PathFinder::~PathFinder()
{
    //locking this will cause a race condition
    //and let's face it we're on the way out
    //so who cares if this gets corrupted? (famous last words)
    m_pendingQueue.clear();
    m_activeQueue.clear();

    m_threadRunning = false;
    m_thread.wait();
}

//public
void PathFinder::setGridSize(const sf::Vector2i& gs)
{
    XY_ASSERT(gs.x > 0 && gs.y > 0, "Invalid grid size");
    m_gridSize = gs;
    m_collisionArray.resize(gs.x * gs.y, 0);
}

void PathFinder::addSolidTile(const sf::Vector2i& tileCoord)
{
    XY_ASSERT(m_gridSize.x > 0 && m_gridSize.y > 0, "Grid size not set");
    m_soldTiles.push_back(tileCoord);
    m_collisionArray[(m_gridSize.x * tileCoord.y) + tileCoord.x] = 1;
}

std::vector<sf::Vector2f> PathFinder::plotPath(const sf::Vector2i& start, const sf::Vector2i& end) const
{
    Node::Ptr currentNode;
    std::set<Node::Ptr> openSet;
    std::set<Node::Ptr> closedSet;
    openSet.insert(std::make_shared<Node>(start));

    while (!openSet.empty())
    {
        currentNode = *openSet.begin();

        for (const auto& node : openSet)
        {
            if (node->getScore() <= currentNode->getScore())
            {
                currentNode = node;
            }
        }

        if (currentNode->position == end)
        {
            break;//we're at the end!
        }

        closedSet.insert(currentNode);
        openSet.erase(std::find(std::begin(openSet), std::end(openSet), currentNode));

        for (auto i = 0u; i < directions.size(); ++i)
        {
            auto coords = currentNode->position + directions[i];

            if (collides(coords) || nodeOnList(closedSet, coords))
            {
                continue;
            }

            auto cost = currentNode->G + ((i < 4u) ? 10u : 14u);
            auto nextNode = nodeOnList(openSet, coords);
            if (!nextNode)
            {
                nextNode = std::make_shared<Node>(coords, currentNode);
                nextNode->G = cost;
                nextNode->H = heuristic/*Diagonal*/(nextNode->position, end);
                openSet.insert(nextNode);
            }
            else if (cost < nextNode->G)
            {
                nextNode->parent = currentNode;
                nextNode->G = cost;
            }
        }
    }

    //climb tree to get our path
    std::vector<sf::Vector2f> points;
    points.reserve(60);
    while (currentNode)
    {
        points.emplace_back(currentNode->position.x * m_tileSize.x, currentNode->position.y * m_tileSize.y);
        points.back() += m_gridOffset;
        currentNode = currentNode->parent;
    }

    return points;
}

void PathFinder::plotPathAsync(const sf::Vector2i& start, const sf::Vector2i& end, std::vector<sf::Vector2f>& dest)
{
    Job job;
    job.start = start;
    job.end = end;
    job.dest = &dest;

    sf::Lock lock(m_mutex);
    m_pendingQueue.push_back(job);
}

bool PathFinder::rayTest(sf::Vector2f start, sf::Vector2f end) const
{
    //based on http://playtechs.blogspot.com/2007/03/raytracing-on-grid.html 

    std::int32_t x0 = static_cast<std::int32_t>(start.x / m_tileSize.x);
    std::int32_t y0 = static_cast<std::int32_t>(start.y / m_tileSize.y);

    std::int32_t x1 = static_cast<std::int32_t>(end.x / m_tileSize.x);
    std::int32_t y1 = static_cast<std::int32_t>(end.y / m_tileSize.y);

    std::int32_t dx = std::abs(x1 - x0);
    std::int32_t dy = std::abs(y1 - y0);
    std::int32_t x = x0;
    std::int32_t y = y0;
    std::int32_t n = 1 + dx + dy;
    std::int32_t x_inc = (x1 > x0) ? 1 : -1;
    std::int32_t y_inc = (y1 > y0) ? 1 : -1;
    std::int32_t error = dx - dy;
    dx *= 2;
    dy *= 2;

    for (; n > 0; --n)
    {
        if (m_collisionArray[(y * m_gridSize.x) + x] == 1)
        {
            //LOG("Found ray collision at " + std::to_string(x) + ", " + std::to_string(y), xy::Logger::Type::Info);
            return true;
        }

        if (error > 0)
        {
            x += x_inc;
            error -= dy;
        }
        else
        {
            y += y_inc;
            error += dx;
        }
    }
    return false;
}

//private
bool PathFinder::collides(const sf::Vector2i& position) const
{
    return (position.x < 0 || position.x >= m_gridSize.x
        || position.y < 0 || position.y >= m_gridSize.y
        || (std::find(m_soldTiles.begin(), m_soldTiles.end(), position) != m_soldTiles.end()));
}

PathFinder::Node::Ptr PathFinder::nodeOnList(const std::set<Node::Ptr>& list, const sf::Vector2i& position) const
{
    auto result = std::find_if(std::begin(list), std::end(list), 
        [&position](const Node::Ptr& node)
    {
        return node->position == position; 
    });
    return (result == list.end()) ? nullptr : *result;
}

void PathFinder::processQueue()
{
    while (m_threadRunning)
    {
        //process entire active queue
        while (!m_activeQueue.empty() && m_threadRunning)
        {
            auto& job = m_activeQueue.back();
            auto result = plotPath(job.start, job.end);

            //std::cout << "Generated " << result.size() << " points\n";

            sf::Lock lock2(m_mutex);
            if (m_threadRunning)
            {
                result.swap(*job.dest);                
            }
            m_mutex.unlock();

            m_activeQueue.pop_back();
        }

        //look for new queue when we're done
        {
            //check to see if we have any more waiting
            sf::Lock lock3(m_mutex);
            if (!m_pendingQueue.empty())
            {
                m_activeQueue.swap(m_pendingQueue);
            }
        }

        //don't suck up all the CPU time
        sf::sleep(sf::milliseconds(500));
    }
}