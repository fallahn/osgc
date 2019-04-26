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

#include "MenuState.hpp"
#include "MenuConsts.hpp"
#include "StateIDs.hpp"
#include "ResourceIDs.hpp"
#include "PluginExport.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_scene         (ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();

    initScene();
    buildMenu();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    quitLoadingScreen();
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

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
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

xy::StateID MenuState::stateID() const
{
    return StateID::MainMenu;
}

//private
void MenuState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
}

void MenuState::buildMenu()
{
    //title
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Drone Drop!");
    entity.getComponent<xy::Text>().setCharacterSize(120);
    entity.addComponent<xy::Drawable>();


    //menu items
    auto itemPos = Menu::ItemFirstPosition;
    //play
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Start Game");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.addComponent<xy::Drawable>();

    auto textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::LeftMouse)
        {
            requestStackClear();
            requestStackPush(StateID::Game);
        }
    });
    itemPos.y += Menu::ItemVerticalSpacing;

    //options
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Options");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.addComponent<xy::Drawable>();

    textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Console::show();
                }
            });
    itemPos.y += Menu::ItemVerticalSpacing;

    //how to play / controls
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("How To Play");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.addComponent<xy::Drawable>();

    textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    LOG("Show the help screen", xy::Logger::Type::Info);
                }
            });
    itemPos.y += Menu::ItemVerticalSpacing;

    //return to launcher
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Return to Launcher");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.addComponent<xy::Drawable>();

    textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    requestStackClear();
                    requestStackPush(StateID::ParentState);
                }
            });
    itemPos.y += Menu::ItemVerticalSpacing;

    //quit
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Quit To Desktop");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.addComponent<xy::Drawable>();

    textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::App::quit();
                    LOG("Enter confirmation here", xy::Logger::Type::Info);
                }
            });
}

void MenuState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}