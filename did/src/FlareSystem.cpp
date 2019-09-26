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

#include "FlareSystem.hpp"
#include "MessageIDs.hpp"
#include "ServerRandom.hpp"
#include "GlobalConsts.hpp"
#include "Actor.hpp"

#include <xyginext/ecs/Scene.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <xyginext/util/Math.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float LaunchOffset = 120.f;
}

FlareSystem::FlareSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(FlareSystem))
{
    requireComponent<Flare>();
    requireComponent<xy::Transform>();
}

//public
void FlareSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& flare = entity.getComponent<Flare>();
        if (flare.state == Flare::Launching)
        {
            flare.stateTime += dt;
            if (flare.stateTime > Flare::LaunchTime)
            {
                flare.stateTime = 0.f;
                flare.state = Flare::Bombarding;
            }
        }
        else if (flare.state == Flare::Bombarding)
        {
            //spawn explosions at random times/positions
            flare.fireTime += dt;
            if (flare.fireTime > Flare::NextFireTime)
            {
                flare.fireTime = Server::getRandomFloat(-0.3f, 0.f);

                auto flarePos = entity.getComponent<xy::Transform>().getPosition();

                auto gridPos = sf::Vector2i(flarePos / Global::TileSize);
                gridPos.x += Server::getRandomInt(-5, 5);
                gridPos.y += Server::getRandomInt(-10, 0);

                gridPos.x = xy::Util::Math::clamp(gridPos.x, 0, static_cast<int>(Global::TileCountX));
                gridPos.y = xy::Util::Math::clamp(gridPos.y, 0, static_cast<int>(Global::TileCountY));

                auto spawnPos = sf::Vector2f(gridPos) * Global::TileSize;

                //don't spawn too close to player
                if (xy::Util::Vector::lengthSquared(spawnPos - flarePos) > 6400) //2.5 tile radius
                {
                    auto* msg = postMessage<ActorEvent>(MessageID::ActorMessage);
                    msg->id = Actor::Explosion;
                    msg->position = spawnPos;
                    //TODO set data to actor id of whoever sent up the flare
                }
            }

            flare.stateTime += dt;
            if (flare.stateTime > Flare::BombardTime)
            {
                getScene()->destroyEntity(entity);
            }
        }
    }
}

void FlareSystem::onEntityAdded(xy::Entity entity)
{

}