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
        VehicleMessage = xy::Message::Count,
        GameMessage
    };
}

struct VehicleEvent final
{
    enum
    {
        RequestRespawn,
        Respawned,
        Fell,
        Exploded,
        LapLine,
        WentAfk
    }type = RequestRespawn;

    xy::Entity entity;
};

struct GameEvent final
{
    enum
    {
        RaceStarted
    }type = RaceStarted;
};