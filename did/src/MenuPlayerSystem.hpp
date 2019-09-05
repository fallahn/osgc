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

#include "InputParser.hpp"

#include <xyginext/ecs/System.hpp>

struct MenuPlayer final
{
    sf::Vector2f velocity;
    bool onSurface = false;
    xy::Entity parent;

    enum Flags
    {
        CanJump = 0x1
    };
    std::uint8_t flags = 0;
};

class MenuPlayerSystem final : public xy::System
{
public:
    explicit MenuPlayerSystem(xy::MessageBus&);

    void process(float) override;

    void reconcile();

private:
    float getDelta(const History&, std::size_t);
    void processInput(std::uint16_t, float, xy::Entity);
};