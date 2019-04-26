/*********************************************************************
(c) Matt Marchant 2017 - 2019
http://trederia.blogspot.com

xygineXT - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "SoundEffectsDirector.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/util/Random.hpp>

#include <array>

namespace
{
    //IDs used to index paths below
    enum AudioID
    {
        Explode, BombDrop,
        BatteryFlat, Noise,
        Pickup, SpawnItem,
        Damage,

        Count
    };

    //paths to audio files - non streaming only
    //must match with enum above
    const std::array<std::string, AudioID::Count> paths = 
    {
        "assets/sound/explode.wav",
        "assets/sound/drop.wav",
        "assets/sound/battery_flat.wav",
        "assets/sound/noise.wav",
        "assets/sound/pickup.wav",
        "assets/sound/spawn_item.wav",
        "assets/sound/damage.wav",
    };

    std::array<std::size_t, AudioID::Count> audioHandles;

    const std::size_t MinEntities = 32;
}

SFXDirector::SFXDirector(xy::ResourceHolder& ar)
    : m_resources       (ar),
    m_nextFreeEntity    (0)
{
    //pre-load sounds
    for (auto i = 0u; i < AudioID::Count; ++i)
    {
        audioHandles[i] = m_resources.load<sf::SoundBuffer>(paths[i]);
    }
}

//public
void SFXDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::BombMessage)
    {
        const auto& data = msg.getData<BombEvent>();
        if (data.type == BombEvent::Dropped)
        {
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::BombDrop]), data.position);
        }
        else if (data.type == BombEvent::Exploded)
        {
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::Explode]), data.position);
        }
    }
    else if (msg.id == MessageID::DroneMessage)
    {
        const auto& data = msg.getData<DroneEvent>();
        switch (data.type)
        {
        default: break;
        case DroneEvent::Died:
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::Explode]), data.position);
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::Noise]), data.position);
            break;
        case DroneEvent::BatteryFlat:
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::BatteryFlat]), data.position);
            break;
        case DroneEvent::GotAmmo:
        case DroneEvent::GotBattery:
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::Pickup]), data.position).setVolume(0.1f);
            break;
        case DroneEvent::Collided:
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::Damage]), data.position);
            break;
        }
    }
    else if (msg.id == MessageID::SpawnMessage)
    {
        const auto& data = msg.getData<SpawnEvent>();
        switch (data.type)
        {
        default: break;
        case SpawnEvent::Collectible:
            playSound(m_resources.get<sf::SoundBuffer>(audioHandles[AudioID::SpawnItem]), data.position);
            break;
        }
    }
}

void SFXDirector::process(float)
{
    //check our ents and free up finished sounds
    for (auto i = 0u; i < m_nextFreeEntity; ++i)
    {
        if (m_entities[i].getComponent<xy::AudioEmitter>().getStatus() == xy::AudioEmitter::Status::Stopped)
        {
            auto entity = m_entities[i];
            m_nextFreeEntity--;
            m_entities[i] = m_entities[m_nextFreeEntity];
            m_entities[m_nextFreeEntity] = entity;
            i--;
        }
    }
}

//private
xy::Entity SFXDirector::getNextFreeEntity()
{
    if (m_nextFreeEntity == m_entities.size())
    {
        resizeEntities(m_entities.size() + MinEntities);
    }
    return m_entities[m_nextFreeEntity++];
}

void SFXDirector::resizeEntities(std::size_t size)
{
    auto currSize = m_entities.size();
    m_entities.resize(size);

    for (auto i = currSize; i < m_entities.size(); ++i)
    {
        m_entities[i] = getScene().createEntity();
        m_entities[i].addComponent<xy::Transform>();
        m_entities[i].addComponent<xy::AudioEmitter>().setChannel(1);// .setSource(m_audioResource.get("placeholder"));
    }
}

xy::AudioEmitter& SFXDirector::playSound(sf::SoundBuffer& buffer, sf::Vector2f position)
{
    auto entity = getNextFreeEntity();
    entity.getComponent<xy::Transform>().setPosition(position);
    auto& emitter = entity.getComponent<xy::AudioEmitter>();
    emitter.setSource(buffer);
    //must reset values here incase they were changed prior to recycling from pool
    emitter.setAttenuation(0.2f);
    emitter.setMinDistance(15.f);
    emitter.setVolume(1.f);
    emitter.setPitch(1.f);
    emitter.play();
    return emitter;
}