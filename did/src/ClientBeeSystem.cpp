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

#include "ClientBeeSystem.hpp"
#include "AnimationSystem.hpp"
#include "Sprite3D.hpp"

#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Vector.hpp>

#include <algorithm>

namespace
{
    const float HeightRadius = 96.f * 96.f; //radius at which bees get full height
    const float MaxBeeHeight = -16.f;
}

ClientBeeSystem::ClientBeeSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ClientBeeSystem))
{
    requireComponent<ClientBees>();
    requireComponent<xy::AudioEmitter>();
    requireComponent<AnimationModifier>();
}

//public
void ClientBeeSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& emitter = entity.getComponent<xy::AudioEmitter>();
        auto& bees = entity.getComponent<ClientBees>();
        const auto& animation = entity.getComponent<AnimationModifier>();

        //set target pitch / volume based on animation
        if (animation.nextAnimation != animation.currentAnimation)
        {
            switch (animation.nextAnimation)
            {
            default: break;
            case AnimationID::IdleDown:
                bees.targetVolume = bees.defaultVolume;
                bees.targetPitch = 1.f;
                break;
            case AnimationID::WalkLeft:
            case AnimationID::WalkRight:
                bees.targetVolume = bees.defaultVolume;
                bees.targetPitch = 1.4f;
                break;
            case AnimationID::WalkDown:
                bees.targetPitch = 0.7f;
                bees.targetVolume = 0.f;
                break;
            case AnimationID::WalkUp:
                bees.targetVolume = bees.defaultVolume;
                bees.targetPitch = 1.f;
                break;
            }
        }

        //update audio parameters over time
        float pitch = emitter.getPitch();
        if (pitch < bees.targetPitch)
        {
            pitch = std::min(bees.targetPitch, pitch + (dt * ClientBees::ChangeSpeed));
        }
        else
        {
            pitch = std::max(bees.targetPitch, pitch - (dt * ClientBees::ChangeSpeed));
        }
        emitter.setPitch(pitch);

        float volume = emitter.getVolume();
        if (volume < bees.targetVolume)
        {
            volume = std::min(bees.targetVolume, volume + (dt/* * ClientBees::ChangeSpeed*/));
        }
        else
        {
            volume = std::max(bees.targetVolume, volume - (dt/* * ClientBees::ChangeSpeed*/));
        }
        emitter.setVolume(volume);

        //see how far away we are from the hive and set the sprite height
        float len2 = xy::Util::Vector::lengthSquared(bees.homePosition - entity.getComponent<xy::Transform>().getPosition());
        len2 = std::min(len2, HeightRadius);

        auto height = MaxBeeHeight * (len2 / HeightRadius);
        entity.getComponent<Sprite3D>().verticalOffset = height;
    }
}