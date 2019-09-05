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

#include "ErrorState.hpp"
#include "SharedStateData.hpp"
#include "MessageIDs.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Image.hpp>

ErrorState::ErrorState(xy::StateStack& stack, xy::State::Context ctx, const SharedStateData& shared)
    : xy::State(stack, ctx),
    m_message(shared.error)
{
    sf::Image img;
    img.create(1, 1, { 0,0,0,160 });
    m_backgroundTexture.loadFromImage(img);
    m_backgroundSprite.setTexture(m_backgroundTexture);
    m_backgroundSprite.setScale(xy::DefaultSceneSize);

    registerWindow(
        [&]()
    {
        XY_ASSERT(xy::App::getRenderWindow(), "no valid window");

        auto windowSize = sf::Vector2f(xy::App::getRenderWindow()->getSize());
        auto boxSize = sf::Vector2f(400.f, 100.f);
        windowSize = (windowSize - boxSize) / 2.f;

        xy::Nim::setNextWindowPosition(windowSize.x, windowSize.y);
        xy::Nim::setNextWindowSize(boxSize.x, boxSize.y);
        xy::Nim::begin("Error");      
        xy::Nim::text(m_message);
        if (xy::Nim::button("OK", 40.f, 16.f))
        {
            requestStackClear();
            requestStackPush(StateID::Menu);
            //getContext().appInstance.getMessageBus().post<MenuEvent>(MessageID::MenuMessage)->action = MenuEvent::QuitGameClicked;
        }
        xy::Nim::end();
    });

    xy::App::setMouseCursorVisible(true);
}

bool ErrorState::handleEvent(const sf::Event&)
{
    return false;
}

void ErrorState::handleMessage(const xy::Message&)
{

}

bool ErrorState::update(float)
{
    return false;
}

void ErrorState::draw()
{
    XY_ASSERT(xy::App::getRenderWindow(), "no valid window");

    auto& rt = *getContext().appInstance.getRenderWindow();
    rt.setView(getContext().defaultView);
    rt.draw(m_backgroundSprite);
}