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
#include "GameConsts.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(1.f);
}

ErrorState::ErrorState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_sharedData(sd)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool ErrorState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    //slight delay before accepting input
    if (delayClock.getElapsedTime() > delayTime)
    {
        if (evt.type == sf::Event::KeyReleased)
        {
            requestStackClear();
            requestStackPush(StateID::MainMenu);
        }
        else if (evt.type == sf::Event::JoystickButtonReleased
            && evt.joystickButton.joystickId == 0)
        {
            if (evt.joystickButton.button == 0)
            {
                requestStackClear();
                requestStackPush(StateID::MainMenu);
            }
        }
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
void ErrorState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);


    auto background = m_resources.load<sf::Texture>("assets/images/"+m_sharedData.theme+"/menu_background.png");
    m_resources.get<sf::Texture>(background).setRepeated(true);

    const float scale = 4.f; //kludge.

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::Depth::Background);
    entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(background));
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();

    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x / scale, 0.f), sf::Vector2f(xy::DefaultSceneSize.x / scale, 0.f));
    verts.emplace_back(xy::DefaultSceneSize / scale, xy::DefaultSceneSize / scale);
    verts.emplace_back(sf::Vector2f(0.f, xy::DefaultSceneSize.y / scale), sf::Vector2f(0.f, xy::DefaultSceneSize.y / scale));
    entity.getComponent<xy::Drawable>().updateLocalBounds();


    auto fontID = m_resources.load<sf::Font>(FontID::GearBoyFont);
    auto& font = m_resources.get<sf::Font>(fontID);

    const sf::Color* colours = GameConst::Gearboy::colours.data();
    if (m_sharedData.theme == "mes")
    {
        colours = GameConst::Mes::colours.data();
    }

    auto createText = [&](const std::string& str)->xy::Entity
    {
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::Depth::Text);
        entity.addComponent<xy::Text>(font).setCharacterSize(GameConst::UI::MediumTextSize);
        entity.getComponent<xy::Text>().setFillColour(colours[0]);
        entity.getComponent<xy::Text>().setOutlineColour(colours[2]);
        entity.getComponent<xy::Text>().setOutlineThickness(2.f);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        entity.getComponent<xy::Text>().setString(str);
        return entity;
    };
    entity = createText("Error");
    entity.getComponent<xy::Transform>().move(0.f, -180.f);

    entity = createText("Failed to load map");
    entity.getComponent<xy::Transform>().move(0.f, -40.f);
}