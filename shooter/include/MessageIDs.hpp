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

#include <xyginext/core/Message.hpp>
#include <xyginext/ecs/Entity.hpp>

namespace MessageID
{
    enum
    {
        BombMessage = xy::Message::Count,
        DroneMessage,
        SpawnMessage,
        MenuMessage
    };
}

struct BombEvent final
{
    enum
    {
        Dropped,
        Exploded,
        DestroyedCollectible,
        DamagedBuilding,
        KilledScorpion,
        KilledBeetle,
        KilledHuman
    }type = Exploded;
    sf::Vector2f position; //top down coordinates
};

struct DroneEvent final
{
    enum
    {
        Spawned,
        Died,
        GotAmmo,
        GotBattery,
        BatteryFlat,
        BatteryLow,
        CollisionStart,
        CollisionEnd
    }type = Spawned;
    std::int32_t lives = 0;
    sf::Vector2f position;
};

struct SpawnEvent final
{
    enum
    {
        Collectible
    }type = Collectible;

    sf::Vector2f position;
};

struct MenuEvent final
{
    enum
    {
        SlideFinished
    }action = SlideFinished;
    xy::Entity entity;
};