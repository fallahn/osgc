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
#include "MessageIDs.hpp"
#include "Actor.hpp"
#include "ResourceIDs.hpp"
#include "InventorySystem.hpp"
#include "ClientWeaponSystem.hpp"
#include "PlayerSystem.hpp"
#include "CarriableSystem.hpp"

#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/Scene.hpp>

#include <xyginext/resources/Resource.hpp>
#include <xyginext/util/Random.hpp>

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

        "assets/sound/effects/decoy_die.wav"
    };

    //std::array<float, AudioID::Count> triggerTimes = {};

    std::array<std::size_t, AudioID::Count> audioHandles = {};

    const std::size_t MinEntities = 32;
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
    else if (msg.id == MessageID::ActorMessage)
    {
        const auto& data = msg.getData<ActorEvent>();
        if (data.type == ActorEvent::Died)
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
            emitter.setVolume(25.f);
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
            switch (data.entity.getComponent<Actor>().id)
            {
            default:break;
            case Actor::ID::PlayerOne:
                playSound(xy::Util::Random::value(AudioID::RodneySpawn01, AudioID::RodneySpawn02),
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
                break;
            case Actor::ID::PlayerTwo:
                playSound(xy::Util::Random::value(AudioID::JeanSpawn01, AudioID::JeanSpawn02),
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
                break;
            case Actor::ID::PlayerThree:
                playSound(xy::Util::Random::value(AudioID::HelenaSpawn01, AudioID::HelenaSpawn02),
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
                break;
            case Actor::ID::PlayerFour:
                playSound(xy::Util::Random::value(AudioID::LarsSpawn01, AudioID::LarsSpawn02),
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
                break;
            }
            break;

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

            switch (data.entity.getComponent<Actor>().id)
            {
            default:break;
            case Actor::ID::PlayerOne:
                playSound(xy::Util::Random::value(AudioID::RodneyDie01, AudioID::RodneyDie04),
                    position).setVolume(45.f);
                break;
            case Actor::ID::PlayerTwo:
                playSound(xy::Util::Random::value(AudioID::JeanDie01, AudioID::JeanDie04),
                    position).setVolume(45.f);
                break;
            case Actor::ID::PlayerThree:
                playSound(xy::Util::Random::value(AudioID::HelenaDie01, AudioID::HelenaDie03),
                    position).setVolume(45.f);
                break;
            case Actor::ID::PlayerFour:
                playSound(xy::Util::Random::value(AudioID::LarsDie01, AudioID::LarsDie04),
                    position).setVolume(45.f);
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
                switch (data.entity.getComponent<Actor>().id)
                {
                default:break;
                case Actor::ID::PlayerOne:
                    playSound(AudioID::RodneyScore01,
                        position).setVolume(45.f);
                    break;
                case Actor::ID::PlayerTwo:
                    playSound(AudioID::JeanScore01,
                        position).setVolume(45.f);
                    break;
                case Actor::ID::PlayerThree:
                    playSound(AudioID::HelenaScore01,
                        position).setVolume(45.f);
                    break;
                case Actor::ID::PlayerFour:
                    playSound(AudioID::LarsScore01,
                        position).setVolume(45.f);
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
            switch (playerID)
            {
            default:break;
            case Actor::ID::PlayerOne:
                playSound(AudioID::RodneyPickup01, 
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
                break;
            case Actor::ID::PlayerTwo:
                playSound(AudioID::JeanPickup01,
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
                break;
            case Actor::ID::PlayerThree:
                playSound(AudioID::HelenaPickup01,
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
                break;
            case Actor::ID::PlayerFour:
                playSound(AudioID::LarsPickup01,
                    data.entity.getComponent<xy::Transform>().getPosition()).setVolume(45.f);
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

    //update the trigger times
    /*for (auto& t : triggerTimes)
    {
        t -= dt;
    }*/
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
    emitter.setSource(m_audioResource.get<sf::SoundBuffer>(audioHandles[audioID]));
    //must reset values here incase they were changed prior to recycling from pool
    emitter.setAttenuation(1.f);
    emitter.setMinDistance(5.f);
    emitter.setVolume(10.f);
    emitter.setPitch(1.f);
    emitter.play();
    return emitter;
}