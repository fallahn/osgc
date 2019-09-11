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

#include "Actor.hpp"
#include "PlayerSystem.hpp"
#include "InventorySystem.hpp"

#include <SFML/System/Vector2.hpp>

struct PlayerInfo final
{
    sf::Vector2f position;    
    Actor actor;
};

struct NetworkState
{
    std::uint32_t serverID = 0;
    std::int32_t actorID = 0;
    sf::Vector2f position;
};

struct ActorState final : public NetworkState
{
    std::int32_t serverTime = 0;
    std::int8_t direction = 0;
};

struct ClientState final : public NetworkState
{
    std::int64_t clientTime = 0;
    Player::Sync sync;
};

struct AnimationState final
{
    std::uint32_t serverID = 0;    
    std::int8_t animation = 0;
};

struct CarriableState final
{
    enum
    {
        PickedUp, Dropped, InWater
    }action;
    std::uint32_t parentID = 0;
    std::uint32_t carriableID = 0;
    sf::Vector2f position;
};

struct InventoryState final
{
    std::uint32_t parentID = 0;    
    Inventory inventory;
};

struct SceneState final
{
    enum
    {
        EntityRemoved,
        PlayerDied,
        PlayerSpawned,
        PlayerDidAction,
        KilledZombie,
        StashedTreasure,
        GotCursed,
        LostCurse,
        Stung
    }action = EntityRemoved;
    std::int32_t serverID = 0;
    sf::Vector2f position;
};

struct ConnectionState final
{
    std::uint64_t clientID = 0;
    Actor::ID actorID = Actor::ID::None;
};

struct RoundStat final
{
    Actor::ID id = Actor::ID::None;
    std::uint16_t treasure = 0;
    std::uint16_t shotsFired = 0;
    std::uint16_t foesSlain = 0;
    std::uint16_t livesLost = 0;
    std::uint16_t roundXP = 0;
};

struct RoundSummary final
{
    RoundStat stats[4];
};

struct PathData final
{
    static constexpr std::size_t Size = 26;
    sf::Vector2f data[Size];
};

struct HoleState final
{
    std::uint32_t serverID = 0;
    std::uint8_t state = 0;
};

struct DebugState final
{
    std::uint32_t serverID = 0;
    std::uint8_t state = 0;
    std::uint8_t pathSize = 0;
    std::int8_t target = -1;
};