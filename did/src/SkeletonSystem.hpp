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

#include <xyginext/ecs/System.hpp>

#include <vector>

namespace Server
{
    struct SharedStateData;
}
class PathFinder;

struct Skeleton final
{
    sf::Vector2f velocity; //movement when struck by weapon
    float timer = 1.2f; //initial time for spawning
    enum State
    {
        Spawning, Walking, Dying//, Thinking
    }state = Spawning;
    bool onFire = false;
    bool inLight = false;
    bool fleeingLight = false;
    std::vector<sf::Vector2f> pathPoints;
    bool pathRequested = false;

    enum Target
    {
        Player,
        Treasure,
        None
    }target = None;
    xy::Entity targetEntity;

    float scanTime = 0.f; //scan every 1s or so for new targets
};

class SkeletonSystem final : public xy::System
{
public:
    SkeletonSystem(xy::MessageBus&, Server::SharedStateData&, PathFinder&);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:
    Server::SharedStateData& m_sharedData;
    std::vector<sf::Vector2f> m_spawnPoints;
    PathFinder& m_pathFinder;

    float m_dayPosition;
    std::size_t m_spawnTimeIndex;
    std::size_t m_spawnPositionIndex;
    std::size_t m_spawnCount;
    float m_spawnTime;

    void updateSpawning(xy::Entity, float);
    void updateNormal(xy::Entity, float);
    void updateDying(xy::Entity, float);
    //void updateThinking(xy::Entity, float);

    void updateCollision(xy::Entity);

    bool isDayTime() const;

    bool setPathToTreasure(xy::Entity);
    void setRandomPath(xy::Entity);

    void onEntityAdded(xy::Entity) override;
    void onEntityRemoved(xy::Entity) override;
};
