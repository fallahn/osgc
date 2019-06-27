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

#include "InputPreviewSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/util/Math.hpp>

#include <SFML/Window/Event.hpp>

namespace
{
    const float RotationAmount = 80.f;
    const float RotationSpeed = 10.f;
}

InputPreviewSystem::InputPreviewSystem(xy::MessageBus& mb)
    : xy::System(mb, typeid(InputPreviewSystem))
{
    requireComponent<InputPreview>();
    requireComponent<xy::Transform>();
}

//public
void InputPreviewSystem::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyPressed)
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            auto& preview = entity.getComponent<InputPreview>();
            if (evt.key.code == preview.inputBinding.keys[InputBinding::Left])
            {
                preview.targetRotation = std::max(-RotationAmount, preview.targetRotation - RotationAmount);
            }
            else if (evt.key.code == preview.inputBinding.keys[InputBinding::Right])
            {
                preview.targetRotation = std::min(RotationAmount, preview.targetRotation + RotationAmount);
            }
        }
    }
    else if (evt.type == sf::Event::KeyReleased)
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            auto& preview = entity.getComponent<InputPreview>();
            if (evt.key.code == preview.inputBinding.keys[InputBinding::Left])
            {
                preview.targetRotation = std::min(RotationAmount, preview.targetRotation + RotationAmount);
            }
            else if (evt.key.code == preview.inputBinding.keys[InputBinding::Right])
            {
               preview.targetRotation = std::max(-RotationAmount, preview.targetRotation - RotationAmount); 
            }
        }
    }
}

void InputPreviewSystem::process(float dt)
{
    auto& entities = getEntities();
    for (auto entity : entities)
    {
        auto& preview = entity.getComponent<InputPreview>();

        if (sf::Joystick::isConnected(preview.inputBinding.controllerID)
            && !((sf::Keyboard::isKeyPressed(preview.inputBinding.keys[InputBinding::Left]) || sf::Keyboard::isKeyPressed(preview.inputBinding.keys[InputBinding::Right]))))
        {
            float stick = RotationAmount * (sf::Joystick::getAxisPosition(preview.inputBinding.controllerID, sf::Joystick::X) / 100.f);

            preview.targetRotation = 
                -std::min(RotationAmount, preview.targetRotation -
                    std::max(preview.targetRotation + stick, -RotationAmount));

            //float hat = RotationAmount * (sf::Joystick::getAxisPosition(preview.inputBinding.controllerID, sf::Joystick::PovX) / 100.f);
            //std::cout << stick << "\n";

            /*preview.targetRotation =
                std::min(RotationAmount, preview.targetRotation -
                    std::max(preview.targetRotation + hat, -RotationAmount));*/
        }

        auto& tx = entity.getComponent<xy::Transform>();

        float rotation = xy::Util::Math::shortestRotation(tx.getRotation(), preview.targetRotation);
        tx.rotate(rotation * RotationSpeed * dt);
    }
}