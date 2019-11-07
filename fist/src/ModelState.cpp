/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "ModelState.hpp"
#include "StateIDs.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <SFML/Window/Event.hpp>

ModelState::ModelState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State (ss, ctx),
    m_uiScene   (ctx.appInstance.getMessageBus())
{
    initScene();
}

//public
bool ModelState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::ui::wantsKeyboard() || xy::ui::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased
        && evt.key.code == sf::Keyboard::Escape)
    {
        xy::App::quit();
    }

    m_uiScene.forwardEvent(evt);
    return true;
}

void ModelState::handleMessage(const xy::Message& msg)
{
    m_uiScene.forwardMessage(msg);
}

bool ModelState::update(float dt)
{
    m_uiScene.update(dt);
    return true;
}

void ModelState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_uiScene);
}

xy::StateID ModelState::stateID() const
{
    return StateID::Model;
}

//private
void ModelState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<xy::CameraSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);

    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setView(getContext().defaultView.getSize());
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setViewport(getContext().defaultView.getViewport());
}