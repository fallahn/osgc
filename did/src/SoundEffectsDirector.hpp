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

#pragma once

#include <xyginext/ecs/Director.hpp>
#include <xyginext/ecs/Entity.hpp>

#include <xyginext/resources/ResourceHandler.hpp>

#include <SFML/System/Vector2.hpp>

#include <vector>
#include <array>

namespace xy
{
    class AudioEmitter;
}

namespace sf
{
    class SoundBuffer;
}

class SFXDirector final : public xy::Director
{
public:

    explicit SFXDirector();

    void handleEvent(const sf::Event&) override {}
    void handleMessage(const xy::Message&) override;
    void process(float) override;

    void mapSpriteIndex(std::int32_t, std::uint8_t);

private:
    xy::ResourceHandler m_audioResource;

    std::vector<xy::Entity> m_entities;
    std::size_t m_nextFreeEntity;

    std::array<std::uint8_t, 5u> m_spriteIndices = {};

    xy::Entity getNextFreeEntity();
    void resizeEntities(std::size_t);
    void addDelay(std::int32_t, sf::Vector2f = {});

    struct AudioDelayTrigger final
    {
        sf::Clock timer;
        sf::Time timeout;
        std::int32_t id = -1;
        sf::Vector2f position;
        float volume = 45.f;
    };
    std::vector<AudioDelayTrigger> m_delays;

    xy::AudioEmitter& playSound(std::int32_t, sf::Vector2f = {}, AudioDelayTrigger = {});
};
