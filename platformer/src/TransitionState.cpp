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

#include "TransitionState.hpp"
#include "SharedStateData.hpp"

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>

TransitionState::TransitionState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State(ss, ctx),
    m_sharedData(sd)
{
    m_sprite.setTexture(sd.transitionContext.texture);
    sd.transitionContext.shader->setUniform("u_texture", sd.transitionContext.texture);
    sd.transitionContext.shader->setUniform("u_screenSize", sf::Glsl::Vec2(sd.transitionContext.texture.getSize()));
}

//public
bool TransitionState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    return false;
}

void TransitionState::handleMessage(const xy::Message&)
{

}

bool TransitionState::update(float dt)
{
    float transitionTime = std::min(1.f, m_transitionClock.getElapsedTime().asSeconds() / m_sharedData.transitionContext.duration);

    m_sharedData.transitionContext.shader->setUniform("u_time", transitionTime);

    if (transitionTime == 1)
    {
        requestStackClear();
        requestStackPush(m_sharedData.transitionContext.nextState);
    }

    return false;
}

void TransitionState::draw()
{
    getContext().renderWindow.setView(getContext().renderWindow.getDefaultView());
    getContext().renderWindow.draw(m_sprite, m_sharedData.transitionContext.shader);
}