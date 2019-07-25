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
#include "EnemySystem.hpp"

#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/resources/Resource.hpp>
#include <xyginext/util/Random.hpp>

#include <array>

namespace
{
    //IDs used to index paths below
    enum AudioID
    {
        Collect, Die, EnemyDie, GetShield,
        Hit, Hit2, Jump, Shoot,

        Count
    };

    //paths to audio files - non streaming only
    //must match with enum above
    const std::array<std::string, AudioID::Count> paths = 
    {
        "assets/sound/collect.wav",
        "assets/sound/die.wav",
        "assets/sound/enemy_die.wav",
        "assets/sound/get_shield.wav",
        "assets/sound/hit.wav",
        "assets/sound/hit2.wav",
        "assets/sound/jump.wav",
        "assets/sound/shoot.wav",        
    };

    std::array<std::size_t, AudioID::Count> audioIDs = {};

    const std::size_t MinEntities = 32;
}

SFXDirector::SFXDirector()
    : m_nextFreeEntity    (0)
{
    //pre-load sounds
    for(auto i = 0; i < AudioID::Count; ++i)
    {
        audioIDs[i] = m_resources.load<sf::SoundBuffer>(paths[i]);
    }
}

//public
void SFXDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        switch (data.type)
        {
        default: break;
        case PlayerEvent::GotCoin:
            playSound(AudioID::Collect, data.entity.getComponent<xy::Transform>().getPosition());
            break;
        case PlayerEvent::Died:
            playSound(AudioID::Die, data.entity.getComponent<xy::Transform>().getPosition());
            break;
        case PlayerEvent::GotShield:
            playSound(AudioID::GetShield, data.entity.getComponent<xy::Transform>().getPosition());
            break;
        case PlayerEvent::Jumped:
            playSound(AudioID::Jump, data.entity.getComponent<xy::Transform>().getPosition());
            break;
        case PlayerEvent::Shot: //TODO swap to star spawn or check ammo level
            playSound(AudioID::Shoot, data.entity.getComponent<xy::Transform>().getPosition());
            break;
        }
    }
    else if (msg.id == MessageID::StarMessage)
    {
        const auto& data = msg.getData<StarEvent>();
        switch (data.type)
        {
        default: break;
        case StarEvent::HitItem:
            playSound(AudioID::Hit, data.position);
            break;
        }
    }
    else if (msg.id == MessageID::EnemyMessage)
    {
        const auto& data = msg.getData<EnemyEvent>();
        switch (data.type)
        {
        default: break;
        case EnemyEvent::SpawnEgg:

            break;
        case EnemyEvent::Died:
            if (data.entity.getComponent<Enemy>().type == Enemy::Bird)
            {
                playSound(AudioID::EnemyDie, data.entity.getComponent<xy::Transform>().getPosition());
            }
            else if (data.entity.getComponent<Enemy>().type != Enemy::Egg)
            {
                
            }
            break;
        }
    }
}

void SFXDirector::process(float dt)
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
        m_entities[i].addComponent<xy::AudioEmitter>();
    }
}

xy::AudioEmitter& SFXDirector::playSound(std::int32_t audioID, sf::Vector2f position)
{
    auto entity = getNextFreeEntity();
    entity.getComponent<xy::Transform>().setPosition(position);
    auto& emitter = entity.getComponent<xy::AudioEmitter>();
    emitter.setSource(m_resources.get<sf::SoundBuffer>(audioIDs[audioID]));
    //must reset values here in case they were changed prior to recycling from pool
    emitter.setAttenuation(0.f);
    emitter.setMinDistance(5.f);
    emitter.setVolume(1.f);
    emitter.setPitch(1.f);
    emitter.setChannel(1);
    emitter.play();
    return emitter;
}