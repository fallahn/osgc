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

#include "DogSystem.hpp"
#include "AnimationSystem.hpp"
#include "GlobalConsts.hpp"
#include "QuadTreeFilter.hpp"
#include "Actor.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/util/Vector.hpp>

namespace
{
    const float Radius = Global::TileSize * 2.f;
    const float RadiusSquare = Radius * Radius;
    const sf::FloatRect SearchArea({ -Radius, -Radius }, { Radius * 2.f, Radius * 2.f });
}

DogSystem::DogSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(DogSystem))
{
    requireComponent<Dog>();
    requireComponent<xy::Transform>();
    requireComponent<AnimationModifier>();
}

//public
void DogSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto dogPosition = entity.getComponent<xy::Transform>().getPosition();
        auto searchArea = SearchArea;
        searchArea.left += dogPosition.x;
        searchArea.top += dogPosition.y;
        auto playerEntities = getScene()->getSystem<xy::DynamicTreeSystem>().query(searchArea, QuadTreeFilter::Player);

        if (playerEntities.empty())
        {
            entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
            auto& emitter = entity.getComponent<xy::AudioEmitter>();
            if (emitter.getStatus() == xy::AudioEmitter::Playing)
            {
                entity.getComponent<xy::AudioEmitter>().pause();
            }
        }
        else
        {
            auto count = 0;
            for (auto ent : playerEntities)
            {
                //don't bark at our owner
                if (ent.getComponent<Actor>().id == entity.getComponent<Dog>().owner)
                {
                    continue;
                }

                auto playerPos = ent.getComponent<xy::Transform>().getPosition();
                auto distance = xy::Util::Vector::lengthSquared(playerPos - dogPosition);
                if (distance < RadiusSquare)
                {
                    count++;
                }
            }

            if (count == 0)
            {
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;

                auto& emitter = entity.getComponent<xy::AudioEmitter>();
                if (emitter.getStatus() == xy::AudioEmitter::Playing)
                {
                    entity.getComponent<xy::AudioEmitter>().pause();
                }
            }
            else
            {
                entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleUp;
                auto& emitter = entity.getComponent<xy::AudioEmitter>();
                if (emitter.getStatus() != xy::AudioEmitter::Playing)
                {
                    entity.getComponent<xy::AudioEmitter>().play();
                }
            }
        }
    }
}