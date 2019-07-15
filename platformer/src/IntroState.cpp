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

#include "IntroState.hpp"
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

#include <fstream>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(1.f);

    sf::Clock textClock;
    const sf::Time textTime = sf::seconds(0.05f);

    xy::Entity textEntity;
}

IntroState::IntroState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_scene         (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_currentLine   (0)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool IntroState::handleEvent(const sf::Event& evt)
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
            nextLine();
        }
        else if (evt.type == sf::Event::JoystickButtonReleased
            && evt.joystickButton.joystickId == 0)
        {
            if (evt.joystickButton.button == 0)
            {
                nextLine();
            }
        }
    }

    m_scene.forwardEvent(evt);

    return false;
}

void IntroState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool IntroState::update(float dt)
{
    m_scene.update(dt);
    return false;
}

void IntroState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void IntroState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    std::ifstream file(xy::FileSystem::getResourcePath() + "assets/dialogue/intro.txt");
    if (file.is_open() && file.good())
    {
        std::string line;
        std::size_t maxLines = 10;

        while (maxLines-- && std::getline(file, line))
        {
            m_lines.push_back(line);
        }
        file.close();

        if (!m_lines.empty())
        {
            auto fontID = m_resources.load<sf::Font>(FontID::GearBoyFont);
            auto& font = m_resources.get<sf::Font>(fontID);

            textEntity = m_scene.createEntity();
            textEntity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
            textEntity.addComponent<xy::Drawable>();
            textEntity.addComponent<xy::Text>(font).setFillColour(sf::Color::White);
            textEntity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::SmallTextSize);
            textEntity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
            textEntity.addComponent<xy::Callback>().active = true;
            textEntity.getComponent<xy::Callback>().userData = std::make_any<std::size_t>(0);
            textEntity.getComponent<xy::Callback>().function =
                [&](xy::Entity entity, float)
            {
                auto& charIndex = std::any_cast<std::size_t&>(entity.getComponent<xy::Callback>().userData);
                if (textClock.getElapsedTime() > textTime
                    && charIndex < m_lines[m_currentLine].size())
                {
                    charIndex++;
                    textClock.restart();
                }

                entity.getComponent<xy::Text>().setString(m_lines[m_currentLine].substr(0, charIndex));
            };
        }
        else
        {
            xy::Logger::log("Skipping intro, no dialogue found", xy::Logger::Type::Error);
            requestStackClear();
            requestStackPush(StateID::Game);
        }
    }
    else
    {
        xy::Logger::log("Skipping intro, dialogue unavailable", xy::Logger::Type::Error);
        requestStackClear();
        requestStackPush(StateID::Game);
    }
}

void IntroState::nextLine()
{
    if (textEntity.isValid() && m_currentLine < m_lines.size() - 1)
    {
        auto& charIndex = std::any_cast<std::size_t&>(textEntity.getComponent<xy::Callback>().userData);

        if (charIndex < m_lines[m_currentLine].size())
        {
            charIndex = m_lines[m_currentLine].size();
        }
        else
        {
            charIndex = 0;
            m_currentLine++;

            //TODO sync images
        }
    }
    else
    {
        requestStackClear();
        requestStackPush(StateID::Game);
    }
}