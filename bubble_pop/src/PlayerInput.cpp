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

#include "PlayerInput.hpp"

#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/core/App.hpp>
#include <xyginext/ecs/components/Transform.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

PlayerInput::PlayerInput()
{

}

//public
void PlayerInput::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::MouseButtonReleased
        && evt.mouseButton.button == sf::Mouse::Left)
    {
        //raise shoot message
    }
}

void PlayerInput::update(float)
{
    auto& rw = *xy::App::getRenderWindow();
    auto mousePos = rw.mapPixelToCoords(sf::Mouse::getPosition(rw));

    auto& tx = m_playerEntity.getComponent<xy::Transform>();
    //sigh this just isn't right that the world position doesn't
    //account for scale and origin offset...
    auto dir = mousePos - (tx.getWorldPosition() + (tx.getOrigin() * 2.f));

    float rotation = xy::Util::Vector::rotation(dir);
    rotation = xy::Util::Math::clamp(rotation, -160.f, -20.f);
    tx.setRotation(rotation);

    //DPRINT("Rotation", std::to_string(rotation));
}