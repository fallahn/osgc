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

//net actors are any entities which are syncronised between client and server
struct NetActor final
{
    std::int32_t serverID = 0;
    std::int32_t actorID = 0;
    std::uint32_t colourID = 4;
    sf::Vector2f velocity;
};

class NetActorSystem final : public xy::System 
{
public:
    explicit NetActorSystem(xy::MessageBus&);

    void process(float) override;

    const std::vector<xy::Entity>& getActors() const { return getEntities(); }
};