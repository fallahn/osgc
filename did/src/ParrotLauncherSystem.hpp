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

namespace Server
{
    struct SharedStateData;
}

struct ParrotLauncher final
{
    float nextLaunchTime = 5.f;
    float currentLaunchTime = 12.f;
    bool launched = false;
};

class ParrotLauncherSystem final : public xy::System
{
public:
    ParrotLauncherSystem(xy::MessageBus& mb, Server::SharedStateData&);

    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:
    Server::SharedStateData& m_sharedData;
    float m_dayTime;
    bool isDay() const;
};