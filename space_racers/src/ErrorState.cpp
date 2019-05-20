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
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

ErrorState::ErrorState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State(ss, ctx),
    m_sharedData(sd),
    m_scene(ctx.appInstance.getMessageBus())
{
    initScene();
}

//public
bool ErrorState::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyPressed
        && evt.key.code == sf::Keyboard::Space)
    {
        requestStackClear();
        requestStackPush(StateID::MainMenu);
    }

    m_scene.forwardEvent(evt);
    return false;
}

void ErrorState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool ErrorState::update(float dt)
{
    m_scene.update(dt);
    
    return false;
}

void ErrorState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void ErrorState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    //background
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-10);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    auto colour = sf::Color(0, 0, 0, 180);
    verts.emplace_back(sf::Vector2f(), colour);
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x, 0.f), colour);
    verts.emplace_back(xy::DefaultSceneSize, colour);
    verts.emplace_back(sf::Vector2f(0.f, xy::DefaultSceneSize.y), colour);

    entity.getComponent<xy::Drawable>().updateLocalBounds();

    //message text
    if (FontID::handles[FontID::Default] == 0)
    {
        FontID::handles[FontID::Default] = m_sharedData.resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");
    }
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -40.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString(m_sharedData.errorMessage);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(48);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 20.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Press Space To Continue");
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setCharacterSize(48);

    auto view = getContext().defaultView;
    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());

    LOG("If there's an error we probably want to be tidying up network resources!", xy::Logger::Type::Warning);
}