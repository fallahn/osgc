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

#include <xyginext/ecs/Entity.hpp>

namespace sf
{
    class Event;
}

struct NodeSet;
class PlayerInput final
{
public:
    explicit PlayerInput(const NodeSet&);

    void handleEvent(const sf::Event&);

    void update(float);

    void setPlayerEntity(xy::Entity e) { m_playerEntity = e; }

private:

    const NodeSet& m_nodeSet;

    xy::Entity m_playerEntity;

    sf::Vector2f getVelocityVector();
};