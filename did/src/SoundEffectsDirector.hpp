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

#include <SFML/System/Vector2.hpp>

#include <vector>

namespace xy
{
    class AudioResource;
    class AudioEmitter;
}

namespace sf
{
    class SoundBuffer;
}

class SFXDirector final : public xy::Director
{
public:

    explicit SFXDirector(xy::AudioResource&);

    void handleEvent(const sf::Event&) override {}
    void handleMessage(const xy::Message&) override;
    void process(float) override;

private:

    xy::AudioResource& m_audioResource;

    std::vector<xy::Entity> m_entities;
    std::size_t m_nextFreeEntity;

    xy::Entity getNextFreeEntity();
    void resizeEntities(std::size_t);
    xy::AudioEmitter& playSound(sf::SoundBuffer&, sf::Vector2f = {});
};
