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

#include "Camera3D.hpp"
#include "AudioSystem.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/audio/Mixer.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/core/App.hpp>

#include <SFML/Audio/Listener.hpp>

namespace
{
    const float CameraDepth = 300.f; //pushes the lister backwards slightly
}

AudioSystem::AudioSystem(xy::MessageBus& mb)
    : System(mb, typeid(AudioSystem))
{
    requireComponent<xy::AudioEmitter>();
    requireComponent<xy::Transform>();
}

//public
void AudioSystem::process(float dt)
{
    //set listener position to active camera
    auto listenerEnt = getScene()->getActiveListener();
    auto listenerPos = listenerEnt.getComponent<xy::Transform>().getWorldTransform().transformPoint({});
    const auto& listener = listenerEnt.getComponent<xy::AudioListener>();

    sf::Listener::setPosition({ listenerPos.x, Camera3D::height, listenerPos.y - (Global::PlayerCameraOffset / 2.f) });
    sf::Listener::setGlobalVolume(listener.getVolume() * xy::AudioMixer::getMasterVolume() * 100.f);

    auto& entities = getEntities();
    for (auto& entity : entities)
    {
        //update position of entities
        const auto& tx = entity.getComponent<xy::Transform>();
        auto& audio = entity.getComponent<xy::AudioEmitter>();

        auto pos = tx.getWorldTransform().transformPoint({});
        audio.setPosition({ pos.x, 0.f, pos.y }); //TODO add a height property to sound emitters
        audio.applyMixerSettings();
        audio.update();
    }
}

//private
void AudioSystem::onEntityRemoved(xy::Entity entity)
{
    auto& ae = entity.getComponent<xy::AudioEmitter>();
    if (ae.hasSource() && ae.getStatus() == xy::AudioEmitter::Playing)
    {
        ae.stop();
    }
}