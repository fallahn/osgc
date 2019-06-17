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

#include "TimeTrialEndState.hpp"
#include "Util.hpp"
#include "GameConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
    //small delay before input works
    const sf::Time delayTime = sf::seconds(0.5f);
}


TimeTrialSummaryState::TimeTrialSummaryState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_shown     (false)
{
    initScene();
    //buildMenu();
}

//public
bool TimeTrialSummaryState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyPressed
        && m_delayClock.getElapsedTime() > delayTime)
    {
        requestStackClear();
        requestStackPush(StateID::MainMenu);
    }

    m_scene.forwardEvent(evt);
    return false;
}

void TimeTrialSummaryState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool TimeTrialSummaryState::update(float dt)
{
    if (!m_shown && m_delayClock.getElapsedTime().asSeconds() > 2.f)
    {
        buildMenu();
        m_shown = true;
    }

    m_scene.update(dt);
    return true;
}

void TimeTrialSummaryState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void TimeTrialSummaryState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);


    auto view = getContext().defaultView;
    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());
}

void TimeTrialSummaryState::buildMenu()
{
    //background
    sf::Color c(0, 0, 0, 220);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-10);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();

    verts.emplace_back(sf::Vector2f(), c);
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x, 0.f), c);
    verts.emplace_back(xy::DefaultSceneSize, c);
    verts.emplace_back(sf::Vector2f(0.f, xy::DefaultSceneSize.y), c);

    entity.getComponent<xy::Drawable>().updateLocalBounds();


    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -480.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Track Record: " + formatTimeString(m_sharedData.trackRecord));
    entity.getComponent<xy::Text>().setCharacterSize(64);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);

    std::string str("Lap Times\n");
    for (auto i = 0u; i < m_sharedData.lapTimes.size(); ++i)
    {
        str += std::to_string(i + 1) + ". " + formatTimeString(m_sharedData.lapTimes[i]) + "\n";
    }
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -370.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString(str);
    entity.getComponent<xy::Text>().setCharacterSize(64);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);

    if (m_sharedData.trackRecord == m_sharedData.lapTimes[0])
    {
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, 220.f);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font).setString("New Track Record!");
        entity.getComponent<xy::Text>().setCharacterSize(76);
        entity.getComponent<xy::Text>().setFillColour(GameConst::PlayerColour::Light[1]);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    }

    //quit message
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 380.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Press Any Key To Continue");
    entity.getComponent<xy::Text>().setCharacterSize(92);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
}