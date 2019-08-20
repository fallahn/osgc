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

#include "AttractState.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/BitmapText.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/BitmapTextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/BitmapFont.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(1.f);
}

AttractState::AttractState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_scene             (ctx.appInstance.getMessageBus()),
    m_sharedData        (sd)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool AttractState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    //slight delay before accepting input
    if (delayClock.getElapsedTime() > delayTime)
    {
        if (evt.type == sf::Event::KeyReleased
            || evt.type == sf::Event::MouseButtonReleased)
        {
            requestStackPop();
        }
    }

    m_scene.forwardEvent(evt);

    return false;
}

void AttractState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool AttractState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void AttractState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void AttractState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::BitmapTextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sharedData.sprites[SpriteID::Title];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 6.f);
    entity.getComponent<xy::Transform>().setScale(2.f, 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);

    auto& font = m_sharedData.resources.get<xy::BitmapFont>(m_sharedData.fontID);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, bounds.height * 2.6f);
    entity.getComponent<xy::Transform>().setScale(2.f, 2.f);

    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::BitmapText>(font).setString("Press Any Key To Start");
    bounds = xy::BitmapText::getLocalBounds(entity);
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    //TODO show high scores
}