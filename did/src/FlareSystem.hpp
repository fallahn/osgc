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

struct Flare final
{
    enum
    {
        Launching, Bombarding
    }state = Launching;

    float stateTime = 0.f;
    static constexpr float LaunchTime = 6.f;
    static constexpr float BombardTime = 4.f;

    float fireTime = 0.f;
    static constexpr float NextFireTime = 0.5f;
};

class FlareSystem final : public xy::System
{
public:
    explicit FlareSystem(xy::MessageBus&);

    void process(float) override;

    void onEntityAdded(xy::Entity) override;

private:

};