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

#include "GameState.hpp"
#include "SeagullSystem.hpp"
#include "CommandIDs.hpp"

#include <xyginext/audio/AudioScape.hpp>
#include <xyginext/audio/Mixer.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>


namespace
{
    const float SeaSoundOffset = 2.f;
    const std::size_t SeaSoundCount = 4;
    const std::array<sf::Vector2f, SeaSoundCount> SeaSoundPositions =
    {
        sf::Vector2f(-SeaSoundOffset, -SeaSoundOffset),
        sf::Vector2f(Global::IslandSize.x + SeaSoundOffset, -SeaSoundOffset),
        sf::Vector2f(Global::IslandSize.x + SeaSoundOffset, Global::IslandSize.y + SeaSoundOffset),
        sf::Vector2f(-SeaSoundOffset, Global::IslandSize.y + SeaSoundOffset)
    };

    const std::size_t SeagullCount = 7;

    const float JungleOffset = 512.f;

    const std::size_t RainSoundCount = 4;
    const std::array<sf::Vector2f, RainSoundCount> RainSoundPositions = 
    {
        sf::Vector2f(Global::IslandSize.x / 2.f, 0.f),
        sf::Vector2f(Global::IslandSize.x, Global::IslandSize.y / 2.f),
        sf::Vector2f(Global::IslandSize.x / 2.f, Global::IslandSize.y),
        sf::Vector2f(0.f, Global::IslandSize.y / 2.f)
    };
}

void GameState::loadAudio()
{ 
    //place sea sources around the edge
    xy::AudioScape as(m_audioResource);
    as.loadFromFile("assets/sound/beach.xas");
    
    for(auto i = 0u; i < SeaSoundCount; ++i)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(SeaSoundPositions[i]);
        entity.addComponent<xy::AudioEmitter>() = as.getEmitter("sea_night");
        entity.addComponent<xy::CommandTarget>().ID = CommandID::LoopedSound;

        auto duration = entity.getComponent<xy::AudioEmitter>().getDuration();
        duration = (duration / static_cast<float>(SeaSoundCount)) * static_cast<float>(i);
        entity.getComponent<xy::AudioEmitter>().setPlayingOffset(duration);

    }

    //add some moving seagulls which randomly chirp during the day
    for (auto i = 0u; i < SeagullCount; ++i)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(Global::IslandSize / 2.f);
        entity.addComponent<xy::AudioEmitter>() = as.getEmitter("seagull0" + std::to_string(i + 1));

        entity.addComponent<Seagull>().currentTime = static_cast<float>(xy::Util::Random::value(7, 15));
        entity.getComponent<Seagull>().callTime = static_cast<float>(xy::Util::Random::value(11, 24));
        sf::Vector2f velocity(xy::Util::Random::value(-1.f, 1.f), xy::Util::Random::value(-1.f, 1.f));
        velocity = xy::Util::Vector::normalise(velocity) * xy::Util::Random::value(120.f, 180.f);
        entity.getComponent<Seagull>().velocity = velocity;

        entity.addComponent<xy::CommandTarget>().ID = CommandID::Bird;
    }


    //create a random distribution of jungle sounds
    auto points = 
        xy::Util::Random::poissonDiscDistribution({ 0.f, 0.f, Global::IslandSize.x - (JungleOffset * 2.f), Global::IslandSize.y - (JungleOffset * 2.f) }, 128.f, 9);

    as.loadFromFile("assets/sound/jungle.xas");

    std::size_t parrotIndex = 0;
    for (auto i = 0u; i < points.size(); ++i)
    {
        auto pos = points[i];
        pos.x += JungleOffset;
        pos.y += JungleOffset;

        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(pos);

        if (i % 2 == 0)
        {
            entity.addComponent<xy::AudioEmitter>() = as.getEmitter("jungle_day");

            auto duration = entity.getComponent<xy::AudioEmitter>().getDuration();
            duration = (duration / 5.f) * static_cast<float>(i);
            entity.getComponent<xy::AudioEmitter>().setPlayingOffset(duration);

            //default volume
            entity.addComponent<float>() = entity.getComponent<xy::AudioEmitter>().getVolume();
            entity.addComponent<xy::CommandTarget>().ID = CommandID::DaySound | CommandID::LoopedSound;

            //add jungle night
            auto nightEnt = m_gameScene.createEntity();
            nightEnt.addComponent<xy::Transform>().setPosition(pos);
            nightEnt.addComponent<xy::AudioEmitter>() = as.getEmitter("jungle_night");

            duration = nightEnt.getComponent<xy::AudioEmitter>().getDuration();
            duration = (duration / 5.f) * static_cast<float>(i);
            nightEnt.getComponent<xy::AudioEmitter>().setPlayingOffset(duration);

            nightEnt.addComponent<float>() = nightEnt.getComponent<xy::AudioEmitter>().getVolume();
            nightEnt.getComponent<xy::AudioEmitter>().setVolume(0.f);
            nightEnt.addComponent<xy::CommandTarget>().ID = CommandID::NightSound | CommandID::LoopedSound;
        }
        else
        {
            switch (parrotIndex)
            {
            default:
            case 0:
                entity.addComponent<xy::AudioEmitter>() = as.getEmitter("parrots01");
                break;
            case 1:
                entity.addComponent<xy::AudioEmitter>() = as.getEmitter("parrots02");
                break;
            case 2:
                entity.addComponent<xy::AudioEmitter>() = as.getEmitter("parrots03");
                break;
            }
            entity.addComponent<Seagull>().currentTime = static_cast<float>(xy::Util::Random::value(7, 15));
            entity.getComponent<Seagull>().callTime = static_cast<float>(xy::Util::Random::value(31, 54));
            entity.getComponent<Seagull>().moves = false;

            entity.addComponent<xy::CommandTarget>().ID = CommandID::Bird;

            parrotIndex = (parrotIndex + 1) % 3;
        }
    }

    as.loadFromFile("assets/sound/weather.xas");

    for (auto i = 0u; i < RainSoundCount; ++i)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(RainSoundPositions[i]);
        entity.addComponent<xy::AudioEmitter>() = as.getEmitter("rain");
        entity.getComponent<xy::AudioEmitter>().setVolume(0.f);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::RainSound | CommandID::LoopedSound;

        auto duration = entity.getComponent<xy::AudioEmitter>().getDuration();
        duration = (duration / static_cast<float>(RainSoundCount)) * static_cast<float>(i);
        entity.getComponent<xy::AudioEmitter>().setPlayingOffset(duration);

    }

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Global::IslandSize / 2.f);
    entity.addComponent<xy::AudioEmitter>() = as.getEmitter("rain");
    entity.getComponent<xy::AudioEmitter>().setVolume(0.f);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::RainSound | CommandID::LoopedSound;

    //day/night music
    as.loadFromFile("assets/sound/music.xas");

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::AudioEmitter>() = as.getEmitter("music_day");
    //default volume for fading back in again
    entity.addComponent<float>() = entity.getComponent<xy::AudioEmitter>().getVolume();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::DaySound | CommandID::LoopedSound;

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::AudioEmitter>() = as.getEmitter("music_night");
    //default volume for fading back in again
    entity.addComponent<float>() = entity.getComponent<xy::AudioEmitter>().getVolume();
    entity.getComponent<xy::AudioEmitter>().setVolume(0.f);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::NightSound | CommandID::LoopedSound;
}