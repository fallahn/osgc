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

#include "ServerPackets.hpp"

#include <xyginext/ecs/System.hpp>

struct DeadReckon final
{
    VehicleActorUpdate update; //this has the same fields as a roid update so can be used for both
    bool hasUpdate = false;
    std::int32_t prevTimestamp = 0;
    std::int32_t lastExtrapolatedTimestamp = 0;
};

class DeadReckoningSystem final : public xy::System
{
public:
    explicit DeadReckoningSystem(xy::MessageBus&);

    void process(float) override;

private:

};