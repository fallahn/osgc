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

#include "ButtonHighlightSystem.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>

#include <SFML/Window/Joystick.hpp>

namespace
{
    const sf::Color buttonColour(80, 80, 80, 255);
}

ButtonHighlightSystem::ButtonHighlightSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(ButtonHighlightSystem))
{
    requireComponent<ButtonHighlight>();
    requireComponent<xy::Sprite>();
    requireComponent<xy::AudioEmitter>();
}

//public
void ButtonHighlightSystem::process(float)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& highlight = entity.getComponent<ButtonHighlight>();
        if (bool pressed = sf::Joystick::isButtonPressed(0, highlight.id); highlight.highlighted != pressed)
        {
            highlight.highlighted = pressed;
            if (pressed)
            {
                entity.getComponent<xy::Sprite>().setColour(buttonColour);
                auto& emitter = entity.getComponent<xy::AudioEmitter>();
                if (emitter.getStatus() == xy::AudioEmitter::Stopped)
                {
                    emitter.play();
                }
            }
            else
            {
                entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
            }
        }
    }
}