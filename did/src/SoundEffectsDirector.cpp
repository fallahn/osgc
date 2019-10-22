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


#include "SoundEffectsDirector.hpp"
#include "MixerChannels.hpp"
#include "MessageIDs.hpp"
#include "Actor.hpp"
#include "ResourceIDs.hpp"
#include "InventorySystem.hpp"
#include "ClientWeaponSystem.hpp"
#include "PlayerSystem.hpp"
#include "CarriableSystem.hpp"
#include "GlobalConsts.hpp"
#include "AudioDelaySystem.hpp"

#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/resources/Resource.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

#include <array>

namespace
{
    enum AudioID
    {
        Pistol01,
        Pistol02,
        Pistol03,
        Pistol04,
        Pistol05,
        Pistol06,

        Dig01,
        Dig02,
        Dig03,

        Sword01,
        Sword02,
        Sword03,
        Sword04,

        BarrelBreak,
        BarrelExplode,

        SwitchPistol,
        SwitchSword,
        SwitchShovel,

        CollectCoin,
        CollectFood,
        Scored,

        MiniMapOpen,
        MiniMapClosed,

        Thunder01,
        Thunder02,
        Thunder03,
        LightningStrike,

        Clash01,
        Clash02,
        Clash03,
        Clash04,

        HelenaDie01,
        HelenaDie02,
        HelenaDie03,
        HelenaScore01,
        HelenaSpawn01,
        HelenaSpawn02,
        HelenaPickup01,

        RodneyDie01,
        RodneyDie02,
        RodneyDie03,
        RodneyDie04,
        RodneyScore01,
        RodneySpawn01,
        RodneySpawn02,
        RodneyPickup01,

        JeanDie01,
        JeanDie02,
        JeanDie03,
        JeanDie04,
        JeanScore01,
        JeanSpawn01,
        JeanSpawn02,
        JeanPickup01,

        LarsDie01,
        LarsDie02,
        LarsDie03,
        LarsDie04,
        LarsScore01,
        LarsSpawn01,
        LarsSpawn02,
        LarsPickup01,

        Splash01,
        Splash02,
        Splash03,
        Splash04,

        DecoyDie,
        Respawn,
        PlayerSpawn,

        Count
    };

    const std::array<std::string, AudioID::Count> paths = 
    {
        "assets/sound/effects/pistol01.wav",
        "assets/sound/effects/pistol02.wav",
        "assets/sound/effects/pistol03.wav",
        "assets/sound/effects/pistol04.wav",
        "assets/sound/effects/pistol05.wav",
        "assets/sound/effects/pistol06.wav",

        "assets/sound/effects/dig01.wav",
        "assets/sound/effects/dig02.wav",
        "assets/sound/effects/dig03.wav",

        "assets/sound/effects/sword01.wav",
        "assets/sound/effects/sword02.wav",
        "assets/sound/effects/sword03.wav",
        "assets/sound/effects/sword04.wav",

        "assets/sound/effects/barrel_break.wav",
        "assets/sound/effects/barrel_explode.wav",

        "assets/sound/effects/switch_pistol.wav",
        "assets/sound/effects/switch_sword.wav",
        "assets/sound/effects/switch_shovel.wav",

        "assets/sound/effects/collect_coin.wav",
        "assets/sound/effects/collect_food.wav",
        "assets/sound/effects/get_treasure.wav",

        "assets/sound/effects/map_open.wav",
        "assets/sound/effects/map_close.wav",

        "assets/sound/effects/thunder01.wav",
        "assets/sound/effects/thunder02.wav",
        "assets/sound/effects/thunder03.wav",
        "assets/sound/effects/lightning_strike.wav",

        "assets/sound/effects/clash01.wav",
        "assets/sound/effects/clash02.wav",
        "assets/sound/effects/clash03.wav",
        "assets/sound/effects/clash04.wav",

        "assets/sound/voice/helena/die01.wav",
        "assets/sound/voice/helena/die02.wav",
        "assets/sound/voice/helena/die03.wav",
        "assets/sound/voice/helena/score01.wav",
        "assets/sound/voice/helena/spawn01.wav",
        "assets/sound/voice/helena/spawn02.wav",
        "assets/sound/voice/helena/grab_treasure01.wav",

        "assets/sound/voice/rodney/die01.wav",
        "assets/sound/voice/rodney/die02.wav",
        "assets/sound/voice/rodney/die03.wav",
        "assets/sound/voice/rodney/die04.wav",
        "assets/sound/voice/rodney/score01.wav",
        "assets/sound/voice/rodney/spawn01.wav",
        "assets/sound/voice/rodney/spawn02.wav",
        "assets/sound/voice/rodney/grab_treasure01.wav",

        "assets/sound/voice/jean/die01.wav",
        "assets/sound/voice/jean/die02.wav",
        "assets/sound/voice/jean/die03.wav",
        "assets/sound/voice/jean/die04.wav",
        "assets/sound/voice/jean/score01.wav",
        "assets/sound/voice/jean/spawn01.wav",
        "assets/sound/voice/jean/spawn02.wav",
        "assets/sound/voice/jean/grab_treasure01.wav",

        "assets/sound/voice/lars/die01.wav",
        "assets/sound/voice/lars/die02.wav",
        "assets/sound/voice/lars/die03.wav",
        "assets/sound/voice/lars/die04.wav",
        "assets/sound/voice/lars/score01.wav",
        "assets/sound/voice/lars/spawn01.wav",
        "assets/sound/voice/lars/spawn02.wav",
        "assets/sound/voice/lars/grab_treasure01.wav",

        "assets/sound/effects/splash01.wav",
        "assets/sound/effects/splash02.wav",
        "assets/sound/effects/splash03.wav",
        "assets/sound/effects/splash04.wav",

        "assets/sound/effects/decoy_die.wav",
        "assets/sound/effects/respawn.wav",
        "assets/sound/effects/player_spawn.wav"
    };

    //std::array<float, AudioID::Count> triggerTimes = {};

    std::array<std::size_t, AudioID::Count> audioHandles = {};

    const std::size_t MinEntities = 32;
    const float VoiceVol = 45.f;
}

SFXDirector::SFXDirector()
    : m_nextFreeEntity    (0)
{
    //pre-load sounds
    for (auto i = 0; i < AudioID::Count; ++i)
    {
        audioHandles[i] = m_audioResource.load<sf::SoundBuffer>(paths[i]);
    }
}

//public
void SFXDirector::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::AnimationMessage)
    {
        const auto& data = msg.getData<AnimationEvent>();
        if (data.entity.hasComponent<Actor>())
        {
            auto id = data.entity.getComponent<Actor>().id;
            auto entity = data.entity;

            switch (id)
            {
            default: break;
            case Actor::ID::PlayerOne:
            case Actor::ID::PlayerTwo:
            case Actor::ID::PlayerThree:
            case Actor::ID::PlayerFour:
            case Actor::ID::Skeleton:
                switch (data.index)
                {
                default:
                    entity.getComponent<xy::AudioEmitter>().pause();
                    break;
                case AnimationID::WalkDown:
                case AnimationID::WalkLeft:
                case AnimationID::WalkRight:
                case AnimationID::WalkUp:
                    if (entity.getComponent<xy::AudioEmitter>().getStatus() != xy::AudioEmitter::Playing)
                    {
                        entity.getComponent<xy::AudioEmitter>().play();
                        entity.getComponent<xy::AudioEmitter>().setVolume(20.f);
                    }
                    break;
                }
                break;
            case Actor::ID::Barrel:
                if (data.index == AnimationID::Barrel::Break)
                {
                    playSound(BarrelBreak,
                        data.entity.getComponent<xy::Transform>().getPosition()).setVolume(20.f);
                }
                else if (data.index == AnimationID::Barrel::Explode)
                {
                    /*playSound(m_audioResource.get(paths[BarrelExplode]),
                        data.entity.getComponent<xy::Transform>().getPosition());*/
                }
                break;
            case Actor::Decoy:
                if (data.index == AnimationID::Die)
                {
                    playSound(DecoyDie,
                        data.entity.getComponent<xy::Transform>().getPosition()).setVolume(20.f);
                }
                break;
            }
        }
        else if (data.entity.hasComponent<ClientWeapon>())
        {
            const auto& weapon = data.entity.getComponent<ClientWeapon>();
            switch (weapon.nextWeapon)
            {
            case Inventory::Pistol:
            {
                auto& emitter = playSound(xy::Util::Random::value(AudioID::Pistol01, AudioID::Pistol06),
                    data.entity.getComponent<xy::Transform>().getWorldPosition());
                emitter.setVolume(20.f);
                emitter.setPitch(xy::Util::Random::value(0.8f, 1.3f));
            }
                break;
            case Inventory::Shovel:
                playSound(xy::Util::Random::value(AudioID::Dig01, AudioID::Dig03),
                    data.entity.getComponent<xy::Transform>().getWorldPosition()).setVolume(5.f);
                break;
            case Inventory::Sword:
                playSound(xy::Util::Random::value(AudioID::Sword01, AudioID::Sword04),
                    data.entity.getComponent<xy::Transform>().getWorldPosition()).setPitch(xy::Util::Random::value(0.9f, 1.3f));
                break;
            default: break;
            }
        }
    }
    else if (msg.id == MessageID::CollectibleMessage)
    {
        const auto& data = msg.getData<CollectibleEvent>();
        if (data.type == CollectibleEvent::Collected)
        {
            switch (data.id)
            {
            default: break;
            case Actor::ID::Coin:
                playSound(AudioID::CollectCoin, data.position).setVolume(25.f);
                break;
            case Actor::ID::Food:
                playSound(AudioID::CollectFood, data.position).setVolume(30.f);
                break;
            case Actor::ID::Ammo:
            {
                auto& emitter = playSound(AudioID::SwitchPistol, data.position);
                emitter.setVolume(30.f);
                emitter.setPitch(1.3f);
            }
            break;
            }
        }
    }    
    else if (msg.id == MessageID::ActorMessage)
    {
        const auto& data = msg.getData<ActorEvent>();
        if (data.type == ActorEvent::Died)
        {
            switch (data.id)
            {
            default: break;
            case Actor::ID::Treasure:
                //if(triggerTimes[AudioID::Scored] < 0)
            {
                //scored in a boat I guess...
                auto& emitter = playSound(AudioID::Scored, data.position);
                emitter.setVolume(30.f);
                emitter.setAttenuation(2.f);

                //triggerTimes[AudioID::Scored] = 0.5f;
            }
                break;
            }
        }
        else if (data.type == ActorEvent::SwitchedWeapon)
        {
            switch (data.id)
            {
            default: break;
            case Inventory::Shovel:
                playSound(AudioID::SwitchShovel, data.position);
                break;
            case Inventory::Pistol:
                playSound(AudioID::SwitchPistol, data.position);
                break;
            case Inventory::Sword:
                playSound(AudioID::SwitchSword, data.position);
                break;
            }
        }
        else if (data.type == ActorEvent::Spawned)
        {
            switch (data.id)
            {
            default: break;
            case Actor::ID::MineSpawn:
                playSound(xy::Util::Random::value(AudioID::Dig01, AudioID::Dig03),
                    data.position).setVolume(5.f);
                break;
            }
        }
    }
    else if (msg.id == MessageID::UIMessage)
    {
        const auto& data = msg.getData<UIEvent>();
        switch (data.action)
        {
        default: break;
        case UIEvent::MiniMapHide:
        case UIEvent::MiniMapShow:
            playSound(AudioID::MiniMapOpen);
            break;
        case UIEvent::MiniMapZoom:
            playSound(AudioID::MiniMapClosed);
            break;
        }
    }
    else if (msg.id == MessageID::MapMessage)
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::LightningFlash)
        {
            auto& emitter = playSound(xy::Util::Random::value(static_cast<int>(AudioID::Thunder01),
                static_cast<int>(AudioID::Thunder03)), data.position);
            
            emitter.setVolume(data.value * 10.f); //tis a kludge for now (and probably forever...)
            emitter.setAttenuation(0.05f);
            emitter.setMinDistance(420.f);
        }
        else if (data.type == MapEvent::LightningStrike)
        {
            auto& emitter = playSound(AudioID::LightningStrike, data.position);
            emitter.setAttenuation(2.f);
            emitter.setMinDistance(4.f);
            emitter.setVolume(25.f);
        }
        else if (data.type == MapEvent::ItemInWater)
        {
            auto& emitter = playSound(xy::Util::Random::value(AudioID::Splash01, AudioID::Splash04), data.position);
            emitter.setAttenuation(2.f);
            emitter.setMinDistance(4.f);
            emitter.setVolume(28.f);
        }
        else if (data.type == MapEvent::ItemRespawn)
        {
            auto& emitter = playSound(AudioID::Respawn, data.position);
            emitter.setAttenuation(2.f);
            emitter.setMinDistance(4.f);
            emitter.setVolume(28.f);
        }
        else if (data.type == MapEvent::Explosion)
        {
            playSound(BarrelExplode, data.position);
        }
    }
    else if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        switch (data.action)
        {
        default:break;
        case PlayerEvent::Drowned:
            //glub.
            break;
        case PlayerEvent::PistolHit:
        case PlayerEvent::ExplosionHit:
        case PlayerEvent::TouchedSkelly:
            //TODO voice ouch or zombie noise
            break;
        case PlayerEvent::SwordHit:
            if (data.entity.getComponent<Inventory>().weapon == Inventory::Sword)
            {
                //clang if we both have swords
                playSound(xy::Util::Random::value(AudioID::Clash01, AudioID::Clash04), 
                    data.entity.getComponent<xy::Transform>().getPosition()).setPitch(xy::Util::Random::value(0.8f, 1.3f));
            }
            else
            {
                //default hit sound
            }
            break;

        case PlayerEvent::Respawned:
        {
            auto position = data.entity.getComponent<xy::Transform>().getPosition();
            switch (m_spriteIndices[data.entity.getComponent<Actor>().id])
            {
            default:break;
            case 0:
            {
                auto id = xy::Util::Random::value(AudioID::RodneySpawn01, AudioID::RodneySpawn02);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            case 1:
            {
                auto id = xy::Util::Random::value(AudioID::JeanSpawn01, AudioID::JeanSpawn02);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            case 2:
            {
                auto id = xy::Util::Random::value(AudioID::HelenaSpawn01, AudioID::HelenaSpawn02);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            case 3:
            {
                auto id = xy::Util::Random::value(AudioID::LarsSpawn01, AudioID::LarsSpawn02);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            }
            playSound(AudioID::PlayerSpawn, data.entity.getComponent<xy::Transform>().getPosition()).setVolume(30.f);
            break;
        }
        case PlayerEvent::Died:
        {
            sf::Vector2f position;
            if (data.entity.hasComponent<Player>())
            {
                //local player
                position = data.entity.getComponent<Player>().deathPosition;
            }
            else
            {
                position = data.position;
            }

            switch (m_spriteIndices[data.entity.getComponent<Actor>().id])
            {
            default:break;
            case 0:
            {
                auto id = xy::Util::Random::value(AudioID::RodneyDie01, AudioID::RodneyDie04);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            case 1:
            {
                auto id = xy::Util::Random::value(AudioID::JeanDie01, AudioID::JeanDie04);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            case 2:
            {
                auto id = xy::Util::Random::value(AudioID::HelenaDie01, AudioID::HelenaDie03);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            case 3:
            {
                auto id = xy::Util::Random::value(AudioID::LarsDie01, AudioID::LarsDie04);
                playSound(id, position).setVolume(VoiceVol);
                addDelay(id, position);
            }
                break;
            }
            break;
        }
        case PlayerEvent::StashedTreasure:
        {
            const auto& actor = data.entity.getComponent<Actor>();
            if (data.data != actor.entityID) //client messages will be the same
            {
                auto position = data.entity.getComponent<xy::Transform>().getPosition();
                switch (m_spriteIndices[data.entity.getComponent<Actor>().id])
                {
                default:break;
                case 0:
                    playSound(AudioID::RodneyScore01, position).setVolume(VoiceVol);
                    addDelay(AudioID::RodneyScore01, position);
                    break;
                case 1:
                    playSound(AudioID::JeanScore01, position).setVolume(VoiceVol);
                    addDelay(AudioID::JeanScore01, position);
                    break;
                case 2:
                    playSound(AudioID::HelenaScore01, position).setVolume(VoiceVol);
                    addDelay(AudioID::HelenaScore01, position);
                    break;
                case 3:
                    playSound(AudioID::LarsScore01, position).setVolume(VoiceVol);
                    addDelay(AudioID::LarsScore01, position);
                    break;
                }

                auto& emitter = playSound(AudioID::Scored, position);
                emitter.setVolume(30.f);
                emitter.setAttenuation(2.f);
            }
            break;
        }
        }
    }
    else if (msg.id == MessageID::CarryMessage)
    {
        const auto& data = msg.getData<CarryEvent>();
        if (data.action == CarryEvent::PickedUp &&
            data.type == Carrier::Treasure)
        {
            auto playerID = data.entity.getComponent<Actor>().id;
            auto position = data.entity.getComponent<xy::Transform>().getPosition();
            switch (m_spriteIndices[playerID])
            {
            default:break;
            case 0:
            {
                playSound(AudioID::RodneyPickup01, position).setVolume(45.f);
                addDelay(AudioID::RodneyPickup01, position);
            }
                break;
            case 1:
            {
                playSound(AudioID::JeanPickup01, position).setVolume(45.f);
                addDelay(AudioID::JeanPickup01, position);
            }
                break;
            case 2:
            {
                playSound(AudioID::HelenaPickup01, position).setVolume(45.f);
                addDelay(AudioID::HelenaPickup01, position);
            }
                break;
            case 3:
            {
                playSound(AudioID::LarsPickup01, position).setVolume(45.f);
                addDelay(AudioID::LarsPickup01, position);
            }
                break;
            }
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

    //play any delayed sounds
    m_delays.erase(std::remove_if(m_delays.begin(), m_delays.end(), 
        [&](const AudioDelayTrigger& d)
        {
            if (d.timer.getElapsedTime() > d.timeout)
            {
                playSound(d.id, d.position, d).setVolume(d.volume);
                return true;
            }

            return false;
        }), m_delays.end());

    //update the trigger times
    /*for (auto& t : triggerTimes)
    {
        t -= dt;
    }*/
}

void SFXDirector::mapSpriteIndex(std::int32_t actor, std::uint8_t index)
{
    XY_ASSERT(actor < m_spriteIndices.size(), "");
    m_spriteIndices[actor] = index;
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
        m_entities[i].addComponent<xy::AudioEmitter>().setChannel(MixerChannel::FX);
        m_entities[i].addComponent<AudioDelay>();
    }
}

xy::AudioEmitter& SFXDirector::playSound(std::int32_t audioID, sf::Vector2f position, AudioDelayTrigger ad)
{
    auto entity = getNextFreeEntity();

    if (ad.id > 0)
    {
        auto& delay = entity.getComponent<AudioDelay>();
        delay.active = true;
        delay.startDistance = xy::Util::Vector::lengthSquared(position - getScene().getActiveListener().getComponent<xy::Transform>().getPosition());
        delay.startVolume = ad.volume;
    }

    entity.getComponent<xy::Transform>().setPosition(position);
    auto& emitter = entity.getComponent<xy::AudioEmitter>();
    emitter.setSource(m_audioResource.get<sf::SoundBuffer>(audioHandles[audioID]));
    //must reset values here incase they were changed prior to recycling from pool
    emitter.setAttenuation(1.f);
    emitter.setMinDistance(5.f);
    emitter.setVolume(10.f);
    emitter.setPitch(1.f);
    emitter.play();
    return emitter;
}

void SFXDirector::addDelay(std::int32_t id, sf::Vector2f position)
{
    const float MaxDelay = 400.f;
    const float MaxDist = xy::Util::Vector::lengthSquared(Global::IslandSize);
    const float MinDist = xy::Util::Vector::lengthSquared(Global::IslandSize / 4.f);

    auto dist = position - getScene().getActiveListener().getComponent<xy::Transform>().getWorldPosition();
    auto len2 = xy::Util::Vector::lengthSquared(dist);

    if (len2 > MinDist)
    {
        auto ratio = std::sqrt(len2) / std::sqrt(MaxDist);

        AudioDelayTrigger ad;
        ad.position = position;
        ad.id = id;
        ad.timeout = sf::milliseconds(static_cast<sf::Int32>(MaxDelay * ratio));
        ad.volume = 34.f * ratio;

        m_delays.push_back(ad);
    }
}