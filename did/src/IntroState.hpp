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

#ifdef STAND_ALONE

#pragma once

#include "StateIDs.hpp"

#include <xyginext/core/State.hpp>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Shader.hpp>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

namespace xy
{
    class MessageBus;
}

class IntroState final : public xy::State
{
public:
    IntroState(xy::StateStack&, Context);
    ~IntroState() = default;

    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

    xy::StateID stateID() const override { return StateID::Intro; }

private:

    sf::Texture m_texture;
    sf::Sprite m_sprite;
    sf::RectangleShape m_rectangleShape;
    sf::Shader m_noiseShader;
    sf::Shader m_lineShader;
    float m_windowRatio;

    xy::MessageBus& m_messageBus;

    float m_fadeTime;
    enum class Fade
    {
        In,
        Hold,
        Out
    }m_fade;

    sf::SoundBuffer m_soundBuffer;
    sf::Sound m_sound;
};

#endif //STAND_ALONE