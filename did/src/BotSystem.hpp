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

#include "InventorySystem.hpp"

#include <xyginext/ecs/System.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Clock.hpp>

#include <random>
#include <deque>

struct Bot final
{
    enum class State
    {
        Searching,
        Digging,
        Fighting,
        Targeting, //has a target such as collectible and moving towards it
        Capturing //ie has treasure, running for home
    }state = State::Searching;

    std::vector<sf::Vector2f> path;
    sf::Vector2f targetPoint;
    
    float acceleration = 1.f;

    std::size_t pointIndex = 0; //index into the array of destination points
    std::size_t indexStride = 1; //how many indices to skip when looking for next point

    float buttonTimer = 0.f; //time until trying to press action button again
    float movementTimer = 0.f; //if this times out we assume we're stuck and need a better path
    float searchTimer = 0.f; //when timed out do a nearby search
    float sweepTimer = 0.f; //which this times out do a sweep search
    float fightTimer = 0.f; //try restarting a fight if this expires
    float itemTimer = 0.f; //try using an item if we're carrying one??

    xy::Entity targetEntity; //might be a hole, treasure or another player
    enum class Target
    {
        None,
        Hole,
        Treasure,
        Light,
        Collectable,
        Enemy,
        Item
    }targetType = Target::None;

    Inventory::Weapon desiredWeapon = Inventory::Pistol;

    std::uint16_t inputMask;
    bool enabled = false;// true;
    bool pathRequested = false; //don't want to request another path while waiting
    bool fleeing = false; //won't fight when we're fleeing
    bool wantsGrab = false; //if we're trying to grab something but didn't get close enough
    std::int8_t previousHealth = 10;//for testing if health changed

    //stores previous states in a stack
    //so bot can return to them when current is complete
    struct StateSettings final
    {
        State state = State::Searching;
        sf::Vector2f targetPoint;
        Target targetType = Target::None;
        xy::Entity targetEntity;
    };
    std::deque<StateSettings> stateStack;

    static constexpr std::size_t MaxStates = 6;
    void pushState()
    {
        StateSettings settings;
        settings.state = state;
        settings.targetPoint = targetPoint;
        settings.targetEntity = targetEntity;
        settings.targetType = targetType;

        stateStack.push_back(settings);
        if (stateStack.size() == MaxStates)
        {
            stateStack.pop_front();
        }
    }

    void popState()
    {
        if (!stateStack.empty())
        {
            auto& settings = stateStack.back();
            state = settings.state;
            targetPoint = settings.targetPoint;
            targetEntity = settings.targetEntity;
            targetType = settings.targetType;

            movementTimer = 0.f;
            fightTimer = 0.f;

            path.clear();
            pathRequested = false;

            stateStack.pop_back();
        }
        else
        {
            //return to default state
            resetState();
        }
    }

    void resetState() 
    {
        state = State::Searching;
        targetEntity = {};
        targetType = Target::None;
        pathRequested = false;
        path.clear();

        movementTimer = 0.f;
        fightTimer = 0.f;

        stateStack.clear();
    }
};

namespace Server
{
    struct SharedStateData;
}

class PathFinder;
class BotSystem final : public xy::System
{
public:
    BotSystem(xy::MessageBus&, PathFinder&, std::size_t);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:
    PathFinder& m_pathFinder;
    std::int32_t m_timeAccumulator;
    std::vector<sf::Vector2f> m_destinationPoints;

    std::mt19937 m_rndEngine;

    float m_dayPosition;
    bool isDay() const;

    void updateSearching(xy::Entity, float);
    void updateDigging(xy::Entity, float);
    void updateFighting(xy::Entity, float);
    void updateCapturing(xy::Entity);
    void updateTargeting(xy::Entity, float);

    void wideSweep(xy::Entity);

    void followPath(xy::Entity);
    void requestPath(xy::Entity);
    void moveToPoint(sf::Vector2f, Bot&);

    void move(sf::Vector2f, sf::Vector2f, Bot&);

    void onEntityAdded(xy::Entity) override;
};