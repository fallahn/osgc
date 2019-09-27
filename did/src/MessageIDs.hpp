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

#include <SFML/System/Vector2.hpp>

namespace MessageID
{
    enum
    {
        AnimationMessage = xy::Message::Count,
        SystemMessage,
        MapMessage,
        ActorMessage,
        PlayerMessage,
        SceneMessage,
        ServerMessage,
        CarryMessage,
        MiniMapUpdate,
        UIMessage
    };
}

struct AnimationEvent final
{
    enum
    {
        Play,
        Stop
    }type;

    sf::Int32 index = -1;
    xy::Entity entity;
};

struct SystemEvent final
{
    enum
    {
        RequestStartServer,
        RequestStopServer,
        ServerStarted,
        ServerStopped
    }action;
};

struct MapEvent final
{
    enum
    {
        Loaded,
        DayNightUpdate,
        HoleAdded,
        LightningStrike,
        LightningFlash,
        ItemInWater,
        FlareLaunched,
        Explosion
    }type;
    float value = 0.f;
    sf::Vector2f position;
};

struct ActorEvent final
{
    enum
    {
        RequestSpawn,
        Spawned,
        Died,
        SwitchedWeapon,
        CollectedItem
    }type;
    sf::Vector2f position;
    std::uint32_t id = 0;
    std::int8_t data = -1;
};

struct PlayerEvent final
{
    enum Action
    {
        DroppedCarrying,
        WantsToCarry,
        NextWeapon,
        PreviousWeapon,
        DidAction,
        PistolHit,
        SwordHit,
        TouchedSkelly,
        ExplosionHit,
        BeeHit,
        Drowned,
        Died,
        Respawned,
        StashedTreasure,
        StoleTreasure,
        ActivatedItem,
        Cursed //data == 0 for uncursed else 1
    }action;
    std::int32_t data = -1;
    sf::Vector2f position; //event might not have occurred at current entity position
    xy::Entity entity;
};

struct SceneEvent final
{
    enum
    {
        CameraLocked,
        CameraUnlocked,
        SlideFinished,
        MessageDisplayFinished,
        WeatherChanged,
        WeatherRequested
    }type = CameraLocked;
    std::uint32_t id = 0;
};

struct ServerEvent final
{
    enum
    {
        ClientConnected,
        ClientDisconnected
    }type;
    std::uint64_t id;
};

struct CarryEvent final
{
    enum
    {
        PickedUp,
        Dropped
    }action;
    xy::Entity entity;
    std::uint8_t type = 0;
};

struct MiniMapEvent final
{
    sf::Vector2f position;
    std::int32_t actorID = 0;
};

struct UIEvent final
{
    enum
    {
        MiniMapShow,
        MiniMapHide,
        MiniMapZoom
    }action;
};