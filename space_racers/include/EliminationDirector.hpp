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
#include <xyginext/ecs/Entity.hpp>

#include <vector>

struct SharedData;
class EliminationDirector final : public xy::Director
{
public:
    EliminationDirector(SharedData&, xy::Scene&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

    void addPlayerEntity(xy::Entity);

private:

    SharedData& m_sharedData;
    xy::Scene& m_uiScene;

    std::vector<xy::Entity> m_playerEntities;

    sf::Clock m_stateTimer;
    enum
    {
        Readying,
        Counting,
        Racing,
        Celebrating,
        GameOver
    }m_state;
    bool m_suddenDeath;
    sf::Clock m_celebrationTimer;

    void eliminate(xy::Entity);
};