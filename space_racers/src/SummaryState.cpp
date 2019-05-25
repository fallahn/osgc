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

#include "SummaryState.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>


#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
    //small delay before input works
    const sf::Time delayTime = sf::seconds(0.5f);
}


SummaryState::SummaryState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus())
{
    initScene();
    buildMenu();
}

//public
bool SummaryState::handleEvent(const sf::Event& evt)
{
    //TODO we want this to return to the lobby
    //rather than the main menu - in which case does
    //the server want to keep the lobby state alive?
    //we also have to remember that the lobby state will
    //try creating a new connection instead of using the
    //existing one....

    if (evt.type == sf::Event::KeyPressed
        && m_delayClock.getElapsedTime() > delayTime)
    {
        /*if (m_sharedData.hosting)
        {
            m_sharedData.server->quit();
        }

        requestStackClear();
        requestStackPush(StateID::MainMenu);*/
        requestStackClear();
        requestStackPush(StateID::Lobby);
    }

    m_scene.forwardEvent(evt);
    return false;
}

void SummaryState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool SummaryState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void SummaryState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void SummaryState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    auto view = getContext().defaultView;
    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());
}

void SummaryState::buildMenu()
{
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


    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, - 80.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Press Any Key To Continue");
    entity.getComponent<xy::Text>().setCharacterSize(96);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
}