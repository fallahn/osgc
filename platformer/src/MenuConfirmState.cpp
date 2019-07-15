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

#include "MenuConfirmState.hpp"
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "CommandIDs.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(2.f);

    const std::array<sf::Vector2f, 2u> selection =
    {
        sf::Vector2f(780.f, 504.f),
        sf::Vector2f(780.f, 604.f)
    };
}

MenuConfirmState::MenuConfirmState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_scene         (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_selectedIndex (0)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool MenuConfirmState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    auto execSelection = [&]()
    {
        if (m_selectedIndex == 0)
        {
            //set up transition
            auto& window = getContext().renderWindow;
            m_sharedData.transitionContext.texture.create(window.getSize().x, window.getSize().y);
            m_sharedData.transitionContext.texture.update(window);
            
            switch (m_sharedData.menuID)
            {
            default: break;
            case MenuID::NewGame:
                m_sharedData.reset();
                m_sharedData.saveProgress();
                m_sharedData.transitionContext.nextState = StateID::Intro;
                requestStackPush(StateID::Transition);
                break;
            case MenuID::Continue:
            case MenuID::GameOver:
                m_sharedData.loadProgress();
                m_sharedData.transitionContext.nextState = StateID::Game;
                requestStackPush(StateID::Transition);
                break;
            }
        }
        else
        {
            switch (m_sharedData.menuID)
            {
            default:
                requestStackPop();
                break;
            case MenuID::GameOver:
            {
                XY_ASSERT(m_sharedData.transitionContext.shader, "shader not set!");
                auto& window = getContext().renderWindow;
                m_sharedData.transitionContext.texture.create(window.getSize().x, window.getSize().y);
                m_sharedData.transitionContext.texture.update(window);
                m_sharedData.transitionContext.nextState = StateID::MainMenu;
                requestStackPush(StateID::Transition);
            }
                break;
            }
        }
    };

    auto selectNext = [&]()
    {
        m_selectedIndex = (m_selectedIndex + 1) % selection.size();
        
        xy::Command cmd;
        cmd.targetFlags = CommandID::Menu::Cursor;
        cmd.action = [&](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition(selection[m_selectedIndex]);
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    };

    //slight delay before accepting input
    if (delayClock.getElapsedTime() > delayTime)
    {
        if (evt.type == sf::Event::KeyPressed)
        {
            switch (evt.key.code)
            {
            default: break;
            case sf::Keyboard::Up:
            case sf::Keyboard::Down:
            case sf::Keyboard::W:
            case sf::Keyboard::S:
                selectNext();
                break;
            case sf::Keyboard::Space:
            case sf::Keyboard::Enter:
                execSelection();
                break;
            }
        }
        else if (evt.type == sf::Event::JoystickButtonPressed
            && evt.joystickButton.joystickId == 0)
        {
            if (evt.joystickButton.button == 0)
            {
                execSelection();
            }
        }
        else if (evt.type == sf::Event::JoystickMoved
            && evt.joystickMove.joystickId == 0)
        {
            switch (evt.joystickMove.axis)
            {
            default: break;
            case sf::Joystick::Y:
            case sf::Joystick::PovY:
                selectNext();
                break;
            }
        }
    }

    m_scene.forwardEvent(evt);

    return false;
}

void MenuConfirmState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool MenuConfirmState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void MenuConfirmState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void MenuConfirmState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);


    auto background = m_resources.load<sf::Texture>("assets/images/"+m_sharedData.theme+"/menu_background.png");
    m_resources.get<sf::Texture>(background).setRepeated(true);

    const float scale = 4.f; //kludge.

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth);
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
        entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
        entity.addComponent<xy::Text>(font).setCharacterSize(GameConst::UI::MediumTextSize);
        entity.getComponent<xy::Text>().setFillColour(colours[0]);
        entity.getComponent<xy::Text>().setOutlineColour(colours[2]);
        entity.getComponent<xy::Text>().setOutlineThickness(2.f);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        entity.getComponent<xy::Text>().setString(str);
        return entity;
    };

    switch (m_sharedData.menuID)
    {
    default: break;
    case MenuID::NewGame:
        entity = createText("Overwrite Save Game?");
        entity.getComponent<xy::Transform>().move(0.f, -180.f);
        break;
    case MenuID::Continue:
        entity = createText("Continue From Save Game?");
        entity.getComponent<xy::Transform>().move(0.f, -180.f);
        break;
    case MenuID::GameOver:
        entity = createText("Restart From Last Checkpoint?");
        entity.getComponent<xy::Transform>().move(0.f, -180.f);

        //add a title
        entity = createText("Game Over");
        entity.getComponent<xy::Transform>().move(0.f, -440.f);
        entity.getComponent<xy::Text>().setOutlineThickness(4.f);
        entity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::LargeTextSize);
        break;
    }

    //yes
    entity = createText("Yes");
    entity.getComponent<xy::Transform>().move(0.f, -40.f);

    //no
    entity = createText("No");
    entity.getComponent<xy::Transform>().move(0.f, 60.f);

    auto arrow = m_resources.load<sf::Texture>("assets/images/"+m_sharedData.theme+"/menu_cursor.png");
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(selection[0]);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(arrow));
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::Cursor;
}