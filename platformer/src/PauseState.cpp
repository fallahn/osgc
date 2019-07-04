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

#include "PauseState.hpp"
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "CommandIDs.hpp"

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

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(1.f);

    const std::array<sf::Vector2f, 3u> selection =
    {
        sf::Vector2f(780.f, 504.f),
        sf::Vector2f(780.f, 604.f),
        sf::Vector2f(780.f, 704.f)
    };

    const std::array<std::string, 3u> messages =
    {
        "Continue Playing",
        "Open The Settings Menu",
        "Unsaved Progress Will Be Lost!"
    };
}

PauseState::PauseState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
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
bool PauseState::handleEvent(const sf::Event& evt)
{
    auto execSelection = [&]()
    {
        switch (m_selectedIndex)
        {
        default: break;
        case 0:
            requestStackPop();
            break;
        case 1:
            xy::Console::show();
            break;
        case 2:
            requestStackClear();
            requestStackPush(StateID::MainMenu);
            break;
        }
    };

    auto selectNext = [&](bool prev = false)
    {
        if (prev)
        {
            m_selectedIndex = (m_selectedIndex + (selection.size() - 1)) % selection.size();
        }
        else
        {
            m_selectedIndex = (m_selectedIndex + 1) % selection.size();
        }

        xy::Command cmd;
        cmd.targetFlags = CommandID::Menu::MenuCursor;
        cmd.action = [&](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition(selection[m_selectedIndex]);
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = CommandID::Menu::MenuMessage;
        cmd.action = [&](xy::Entity e, float)
        {
            e.getComponent<xy::Text>().setString(messages[m_selectedIndex]);
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    };

    //slight delay before accepting input
    if (delayClock.getElapsedTime() > delayTime)
    {
        if (evt.type == sf::Event::KeyReleased)
        {
            switch (evt.key.code)
            {
            default: break;
            case sf::Keyboard::Up:
            case sf::Keyboard::W:
                selectNext(true);
                break;
            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                selectNext();
                break;
            case sf::Keyboard::Space:
            case sf::Keyboard::Enter:
                execSelection();
                break;
            }
        }
        else if (evt.type == sf::Event::JoystickButtonReleased
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
#ifdef _WIN32
                //POVY is inverted on windows
                bool prev = ((evt.joystickMove.position < 0 && evt.joystickMove.axis == sf::Joystick::Y)
                    || (evt.joystickMove.position > 0 && evt.joystickMove.axis == sf::Joystick::PovY));
                selectNext(prev);
#else
                selectNext((evt.joystickMove.position < 0));
#endif //_WIN32
                break;
            }
        }
    }

    m_scene.forwardEvent(evt);

    return false;
}

void PauseState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool PauseState::update(float dt)
{
    m_scene.update(dt);
    return false;
}

void PauseState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void PauseState::build()
{
    //TODO load correct graphics / colours based on current theme

    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);


    auto background = m_resources.load<sf::Texture>("assets/images/gearboy/menu_background.png");
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


    auto fontID = m_resources.load<sf::Font>("assets/fonts/IBM_CGA.ttf");
    auto& font = m_resources.get<sf::Font>(fontID);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -180.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(64);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setString("Paused");


    //continue
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -40.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(64);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setString("Continue");

    //options
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 60.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(64);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setString("Options");

    //quit
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 160.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(64);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setString("Quit");

    //selection message
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 460.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(64);
    entity.getComponent<xy::Text>().setFillColour(GameConst::Gearboy::colours[0]);
    entity.getComponent<xy::Text>().setOutlineColour(GameConst::Gearboy::colours[2]);
    entity.getComponent<xy::Text>().setOutlineThickness(2.f);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::MenuMessage;

    auto arrow = m_resources.load<sf::Texture>("assets/images/gearboy/menu_cursor.png");
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(selection[0]);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(arrow));
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::MenuCursor;
}