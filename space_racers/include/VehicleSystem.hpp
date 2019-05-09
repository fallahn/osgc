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

#include <array>

struct Input final
{
    sf::Uint16 mask = 0;
    sf::Int64 timestamp = 0;
};

using History = std::array<Input, 120>;

struct Vehicle final
{
    History history;
    std::size_t currentInput = 0;
    std::size_t lastUpdateInput = history.size() - 1;


};

class VehicleSystem final : public xy::System 
{
public: 
    explicit VehicleSystem(xy::MessageBus& mb);

    void process(float) override;

private:
};