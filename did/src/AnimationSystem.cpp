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

#include "AnimationSystem.hpp"
#include "Actor.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/SpriteAnimation.hpp>

AnimationSystem::AnimationSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(AnimationSystem))
{
    requireComponent<AnimationModifier>();
    requireComponent<xy::SpriteAnimation>();
}

//public
void AnimationSystem::handleMessage(const xy::Message&)
{

}

void AnimationSystem::process(float)
{
    auto& entities = getEntities();
    for (auto& entity : entities)
    {
        auto& am = entity.getComponent<AnimationModifier>();
        if (am.nextAnimation != am.currentAnimation)
        {
            entity.getComponent<xy::SpriteAnimation>().play(am.animationMap[am.nextAnimation]);
            entity.getComponent<xy::SpriteAnimation>().setFrameID(0);
            am.currentAnimation = am.nextAnimation;

            auto* msg = postMessage<AnimationEvent>(MessageID::AnimationMessage);
            msg->entity = entity;
            msg->type = AnimationEvent::Play;
            msg->index = am.currentAnimation;
        }
    }
}