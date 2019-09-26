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

#include <xyginext/ecs/Director.hpp>

namespace Server
{
    struct SharedStateData;
}

struct WetPatch final
{
    enum State
    {
        Zero, ThirtyThree, SixtySix, OneHundred
    }state = State::Zero;
    sf::Uint32 digCount = 0;
    std::int32_t owner = -1; //this might be a booby trap
    static constexpr sf::Uint32 DigsPerStage = 3;
};

class DigDirector final : public xy::Director
{
public:
    explicit DigDirector(Server::SharedStateData&);

    void handleMessage(const xy::Message&) override;
    void handleEvent(const sf::Event&) override;
    void process(float) override;

private:
    Server::SharedStateData& m_sharedData;
};