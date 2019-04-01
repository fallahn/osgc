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

#include "MenuState.hpp"
#include "States.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedStateData& sd)
    : xy::State (ss,ctx),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_sharedData(sd)
{
    createScene();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    ctx.appInstance.setMouseCursorVisible(true);
}

bool MenuState::handleEvent(const sf::Event& evt)
{
    m_scene.getSystem<xy::UISystem>().handleEvent(evt);
    m_scene.forwardEvent(evt);
    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void MenuState::draw()
{
    auto rw = getContext().appInstance.getRenderWindow();
    rw->draw(m_scene);
}

xy::StateID MenuState::stateID() const
{
    return States::MenuState;
}

void MenuState::createScene()
{
    //add the systems
    auto& messageBus = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::TextSystem>(messageBus);
    m_scene.addSystem<xy::UISystem>(messageBus);
    m_scene.addSystem<xy::RenderSystem>(messageBus);
}









