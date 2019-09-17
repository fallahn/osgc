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
#include "ResourceIDs.hpp"
#include "GlobalConsts.hpp"

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
    const sf::Time delayTime = sf::seconds(1.f);

    namespace CommandID
    {
        enum
        {
            Message = 0x1,
            Cursor = 0x2,
        };
    }

    const std::array<sf::Vector2f, 3u> selection =
    {
        sf::Vector2f(720.f, 464.f),
        sf::Vector2f(720.f, 564.f),
        sf::Vector2f(720.f, 664.f)
    };

    const std::array<std::string, 3u> messages =
    {
        "Continue Playing",
        "Open The Settings Menu",
        "If you are hosting this will stop the game"
    };
}

PauseState::PauseState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_scene             (ctx.appInstance.getMessageBus()),
    m_sharedData        (sd),
    m_selectedIndex     (0)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool PauseState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::ui::wantsKeyboard() || xy::ui::wantsMouse())
    {
        return false;
    }

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
            m_sharedData.netClient->disconnect();
            if (m_sharedData.gameServer->running())
            {
                //I wish there were a way to signal the other clients
                //first but the server closes before the packets are
                //sent. Might have to look at implementing this in
                //the server class itself...
                m_sharedData.gameServer->stop();
            }

            requestStackClear();
            requestStackPush(StateID::Menu);
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
        cmd.targetFlags = CommandID::Cursor;
        cmd.action = [&](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition(selection[m_selectedIndex]);
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = CommandID::Message;
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
            case sf::Keyboard::Escape:
#ifdef XY_DEBUG
                xy::App::quit();
#else
                requestStackPop();
#endif
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
            case sf::Joystick::PovY:
                if (std::abs(evt.joystickMove.position) > 20)
                {
//#ifdef _WIN32
//                    //POVY is inverted on windows
//                    bool prev = ((evt.joystickMove.position > 0 && evt.joystickMove.axis == sf::Joystick::Y)
//                        || (evt.joystickMove.position < 0 && evt.joystickMove.axis == sf::Joystick::PovY));
//                    selectNext(prev);
//#else
                    selectNext((evt.joystickMove.position > 0));
//#endif //_WIN32
                }
                break;
            }
        }
        /*else if (evt.type == sf::Event::MouseMoved)
        {
            if (std::abs(evt.mouseMove.x) > 10)
            {
                selectNext(evt.mouseMove.x > 0);
            }
        }*/
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
    return true;
}

void PauseState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void PauseState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    //background
    auto background = m_resources.load<sf::Texture>("assets/images/summary_window.png");

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(-100);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(background));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    //text
    auto fontID = m_resources.load<sf::Font>(Global::FineFont);
    auto& font = m_resources.get<sf::Font>(fontID);

    auto createText = [&](const std::string& str)->xy::Entity
    {
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.addComponent<xy::Drawable>().setDepth(0);
        entity.addComponent<xy::Text>(font).setCharacterSize(Global::MediumTextSize);
        entity.getComponent<xy::Text>().setFillColour(Global::InnerTextColour);
        entity.getComponent<xy::Text>().setOutlineColour(Global::OuterTextColour);
        entity.getComponent<xy::Text>().setOutlineThickness(2.f);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        entity.getComponent<xy::Text>().setString(str);
        return entity;
    };
    entity = createText("Paused");
    entity.getComponent<xy::Transform>().move(0.f, -340.f);
    entity.getComponent<xy::Text>().setCharacterSize(Global::LargeTextSize);
    entity.getComponent<xy::Text>().setOutlineThickness(4.f);

    //continue
    entity = createText("Continue");
    entity.getComponent<xy::Transform>().move(0.f, -100.f);

    //options
    entity = createText("Options");
    entity.getComponent<xy::Transform>().move(0.f, 0.f);

    //quit
    entity = createText("Quit");
    entity.getComponent<xy::Transform>().move(0.f, 100.f);

    //selection message
    entity = createText("Continue Playing");
    entity.getComponent<xy::Transform>().move(0.f, 260.f);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Message;
    entity.getComponent<xy::Text>().setCharacterSize(Global::SmallTextSize + 8);

    auto arrow = m_resources.load<sf::Texture>("assets/images/menu_cursor.png");
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(selection[0]);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(arrow));
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Cursor;
}