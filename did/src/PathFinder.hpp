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

#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Thread.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/Config.hpp>

#include <vector>
#include <memory>
#include <set>
#include <atomic>

class PathFinder final
{
public:
    PathFinder();

    ~PathFinder();
    PathFinder(PathFinder&&) = delete;
    PathFinder(const PathFinder&) = delete;
    const PathFinder& operator = (PathFinder&&) = delete;
    const PathFinder& operator = (const PathFinder&) = delete;

    //set the tile count in the x and y directions
    void setGridSize(const sf::Vector2i& gs) { m_gridSize = gs; }
    //set the size of a tile in world units
    void setTileSize(const sf::Vector2f& ts) { m_tileSize = ts; }
    //applies an offset from the default node position of tile centre
    void setGridOffset(const sf::Vector2f& go) { m_gridOffset = go; }
    //add the x/y position of a solid tile
    void addSolidTile(const sf::Vector2i& st);
    //returns false if no mapp data has yet been supplied
    bool hasData() const { return !m_soldTiles.empty() && m_gridSize.x != 0 && m_gridSize.y != 0; }
    //returns a vector of points in world coords plotting a path between the
    //start tile and end tile (given in tile coords)
    std::vector<sf::Vector2f> plotPath(const sf::Vector2i&, const sf::Vector2i&) const;
    //plots a path asyncronously to prevent blocking
    void plotPathAsync(const sf::Vector2i&, const sf::Vector2i&, std::vector<sf::Vector2f>&);

private:
    struct Node final
    {
        using Ptr = std::shared_ptr<Node>;
        Node(const sf::Vector2i& pos, Node::Ptr p = nullptr)
        : position (pos), parent(p), G(0), H(0){}

        sf::Uint32 getScore() const
        {
            return G + H;
        }

        sf::Vector2i position;
        Node::Ptr parent;
        sf::Uint32 G, H;
    };

    sf::Vector2i m_gridSize;
    sf::Vector2f m_tileSize;
    sf::Vector2f m_gridOffset;
    std::vector<sf::Vector2i> m_soldTiles;

    bool collides(const sf::Vector2i&) const;
    Node::Ptr nodeOnList(const std::set<Node::Ptr>&, const sf::Vector2i&) const;

    sf::Thread m_thread;
    sf::Mutex m_mutex;
    std::atomic_bool m_threadRunning;
    void processQueue();

    struct Job final
    {
        sf::Vector2i start;
        sf::Vector2i end;
        std::vector<sf::Vector2f>* dest = nullptr;
    };
    std::vector<Job> m_activeQueue;
    std::vector<Job> m_pendingQueue;
};

inline bool operator == (const sf::Vector2i& lh, const sf::Vector2u& rh)
{
    return (lh.x == static_cast<int>(rh.x) && lh.y == static_cast<int>(rh.y));
}

inline bool operator != (const sf::Vector2i& lh, const sf::Vector2u& rh)
{
    return !(lh == rh);
}
