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
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>

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
    xy::Entity spriteEntity;
}

IntroState::IntroState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_scene         (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_currentLine   (0),
    m_currentAction (0)
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
    m_actions[m_currentAction].update(dt);

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
    m_scene.addSystem<xy::SpriteSystem>(mb);
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
            textEntity.getComponent<xy::Transform>().move(0.f, 480.f);
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

            //sprite
            xy::SpriteSheet spriteSheet;
            spriteSheet.loadFromFile("assets/sprites/intro.spt", m_resources);

            spriteEntity = m_scene.createEntity();
            spriteEntity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
            spriteEntity.addComponent<xy::Drawable>();
            spriteEntity.addComponent<xy::Sprite>() = spriteSheet.getSprite("background");
            spriteEntity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
            auto bounds = spriteEntity.getComponent<xy::Sprite>().getTextureBounds();
            spriteEntity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
            spriteEntity.getComponent<xy::Transform>().setScale(4.f, 4.f);

            auto screwDriverEnt = m_scene.createEntity();
            screwDriverEnt.addComponent<xy::Transform>().setPosition(600.f, -512.f);
            screwDriverEnt.addComponent<xy::Drawable>().setDepth(1);
            screwDriverEnt.addComponent<xy::Sprite>() = spriteSheet.getSprite("screwdriver");
            screwDriverEnt.getComponent<xy::Transform>().setScale(4.f, 4.f);

            //action list
            m_actions.resize(7);
            m_actionTimes = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
            //fades in sprite
            auto& actionOne = m_actions[0];
            actionOne.update = [&](float dt)
            {
                if (!actionOne.finished)
                {
                    float& currTime = m_actionTimes[0];

                    //fade in
                    currTime = std::min(1.f, currTime + dt);
                    auto c = sf::Color::White;
                    c.a = static_cast<sf::Uint8>(255.f * currTime);
                    spriteEntity.getComponent<xy::Sprite>().setColour(c);

                    if (currTime == 1)
                    {
                        actionOne.finished = true;
                    }
                }
            };

            actionOne.finish = [&]()
            {
                spriteEntity.getComponent<xy::Sprite>().setColour(sf::Color::White);
                actionOne.finished = true;
            };

            //slide to next screen
            bounds.left += bounds.width;
            auto& actionTwo = m_actions[1];
            actionTwo.update = [&, bounds](float dt)
            {
                if (!actionTwo.finished)
                {
                    auto currBounds = spriteEntity.getComponent<xy::Sprite>().getTextureRect();
                    float move = bounds.left - currBounds.left;
                    
                    if (move < 3)
                    {
                        spriteEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
                        actionTwo.finished = true;
                    }
                    else
                    {
                    currBounds.left += move * dt * 4.f;
                    spriteEntity.getComponent<xy::Sprite>().setTextureRect(currBounds);
                    }
                }
            };

            actionTwo.finish = [&, bounds]
            {
                spriteEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
                actionTwo.finished = true;
            };

            //slide down
            bounds.top += bounds.height;
            auto& actionThree = m_actions[2];
            actionThree.update = [&, bounds](float dt)
            {
                if (!actionThree.finished)
                {
                    auto currBounds = spriteEntity.getComponent<xy::Sprite>().getTextureRect();
                    float move = bounds.top - currBounds.top;

                    if (move < 3)
                    {
                        spriteEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
                        actionThree.finished = true;
                    }
                    else
                    {
                        currBounds.top += move * dt * 4.f;
                        spriteEntity.getComponent<xy::Sprite>().setTextureRect(currBounds);
                    }
                }
            };

            actionThree.finish = [&, bounds]
            {
                spriteEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
                actionThree.finished = true;
            };

            //slide back
            bounds.left -= bounds.width;
            auto& actionFour = m_actions[3];
            actionFour.update = [&, bounds](float dt)
            {
                if (!actionFour.finished)
                {
                    auto currBounds = spriteEntity.getComponent<xy::Sprite>().getTextureRect();
                    float move = bounds.left - currBounds.left;

                    if (move > -3)
                    {
                        spriteEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
                        actionFour.finished = true;
                    }
                    else
                    {
                        currBounds.left += move * dt * 4.f;
                        spriteEntity.getComponent<xy::Sprite>().setTextureRect(currBounds);
                    }
                }
            };

            actionFour.finish = [&, bounds]
            {
                spriteEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
                actionFour.finished = true;
            };

            //insert screwdriver
            auto& actionFive = m_actions[4];
            actionFive.update =
                [&, screwDriverEnt](float dt) mutable
            {
                if (!actionFive.finished &&
                    screwDriverEnt.getComponent<xy::Transform>().getPosition().y < 0)
                {
                    screwDriverEnt.getComponent<xy::Transform>().move(0.f, 220.f * dt);
                }
                else
                {
                    actionFive.finished = true;
                }
            };
            actionFive.finish =
                [&, screwDriverEnt]() mutable
            {
                auto pos = screwDriverEnt.getComponent<xy::Transform>().getPosition();
                pos.y = 0.f;
                screwDriverEnt.getComponent<xy::Transform>().setPosition(pos);
                actionFive.finished = true;
            };

            //elec shock
            auto& actionSix = m_actions[5];
            actionSix.update = [&](float dt)
            {
                if (!actionSix.finished)
                {
                    //TODO display shock lines

                    auto pos = xy::DefaultSceneSize / 2.f;
                    pos.x += xy::Util::Random::value(-5.f, 5.f);
                    pos.y += xy::Util::Random::value(-5.f, 5.f);
                    spriteEntity.getComponent<xy::Transform>().setPosition(pos);

                    float& currTime = m_actionTimes[5];
                    currTime += dt;
                    if (currTime > 2.5f)
                    {
                        actionSix.finish();
                    }
                }
            };

            actionSix.finish = 
                [&, screwDriverEnt]() mutable
            {
                screwDriverEnt.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
                spriteEntity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
                textEntity.getComponent<xy::Text>().setFillColour(sf::Color::Transparent);
                actionSix.finished = true;

                m_currentAction++;
            };

            //pause
            auto& actionSeven = m_actions[6];
            actionSeven.update = [&](float dt)
            {
                float& currTime = m_actionTimes[6];
                currTime += dt;
                if (currTime > 3.f)
                {
                    requestStackClear();
                    requestStackPush(StateID::Game);
                }
            };
            actionSeven.finish = [&]()
            {
                requestStackClear();
                requestStackPush(StateID::Game);
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

            m_actions[m_currentAction].finish();
        }
        else
        {
            charIndex = 0;
            m_currentLine++;

            //sync images
            if (m_currentAction < m_actions.size() - 1)
            {
                if (!m_actions[m_currentAction].finished)
                {
                    m_actions[m_currentAction].finish();
                }

                m_currentAction++;
            }
        }
    }
    else
    {
        if (m_currentAction < m_actions.size() - 1)
        {
            m_currentAction++;
        }
        else
        {
            m_actions[m_currentAction].finish();
        }
    }
}