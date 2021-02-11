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

#include "GameOverState.hpp"
#include "IniParse.hpp"
#include "GameConsts.hpp"
#include "CommandID.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/BitmapText.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/BitmapTextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/BitmapFont.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(1.f);

    const std::vector<std::string> names =
    {
        "Barry Lotter",
        "Eddy Whirlpool",
        "Buns McFarlane",
        "Enty Hannah",
        "Jeff Grapes",
        "LZ Cook"
    };
}

GameOverState::GameOverState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_scene             (ctx.appInstance.getMessageBus()),
    m_sharedData        (sd)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool GameOverState::handleEvent(const sf::Event& evt)
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
            switch (evt.key.code)
            {
            default: break;
            case sf::Keyboard::BackSpace:
                if (!m_playerName.empty())
                {
                    m_playerName.pop_back();
                    updatePlayerString();
                }
                break;
            case sf::Keyboard::Return:

                if (m_playerName.empty())
                {
                    m_playerName = names[xy::Util::Random::value(0u, names.size() - 1)];
                }

                if (m_sharedData.score > m_sharedData.highScores.back().second)
                {
                    std::int32_t i = m_sharedData.highScores.size() - 1;
                    while (i > 0 && m_sharedData.highScores[i - 1].second < m_sharedData.score)
                    {
                        m_sharedData.highScores[i] = m_sharedData.highScores[i - 1];
                        i--;
                    }
                    m_sharedData.highScores[i] = std::make_pair(m_playerName, static_cast<std::int32_t>(m_sharedData.score));

                    //write score file
                    IniParse ini;
                    for (const auto& [name, score] : m_sharedData.highScores)
                    {
                        ini.setValue("scores", name, score);
                    }

                    ini.write(xy::FileSystem::getConfigDirectory(xy::App::getActiveInstance()->getApplicationName()) + "scores.ini");
                }

                requestStackPop();
                break;
            }
        }
        else if (evt.type == sf::Event::TextEntered)
        {
            //enter name into high score string - bitmap text is ASCII only so...
            if (evt.text.unicode > 31
                && evt.text.unicode < 127
                && m_playerName.size() < Const::MaxNameChar)
            {
                m_playerName += static_cast<char>(evt.text.unicode);
                updatePlayerString();
            }
        }
    }

    m_scene.forwardEvent(evt);

    return false;
}

void GameOverState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool GameOverState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void GameOverState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void GameOverState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::BitmapTextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sharedData.sprites[SpriteID::GameOver];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 2.f);
    entity.getComponent<xy::Transform>().setScale(2.f, 2.f);

    auto& font = m_sharedData.resources.get<xy::BitmapFont>(m_sharedData.fontID);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 260.f);
    entity.getComponent<xy::Transform>().setScale(2.f, 2.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::BitmapText>(font).setString("Press Enter To Continue");
    bounds = xy::BitmapText::getLocalBounds(entity);
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    //show high scores input
    if (m_sharedData.score >= m_sharedData.highScores.back().second)
    {
        //background
        entity = m_scene.createEntity();
        entity.addComponent<xy::Drawable>().setDepth(-10);
        entity.addComponent<xy::Sprite>() = m_sharedData.sprites[SpriteID::NameInput];
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.addComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, bounds.height * 2.f);
        entity.getComponent<xy::Transform>().setScale(2.f, 2.f);
        auto boxPos = entity.getComponent<xy::Transform>().getPosition();

        //header
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, -bounds.height * 2.2f);
        entity.getComponent<xy::Transform>().setScale(2.f, 2.f);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::BitmapText>(font).setString("Enter Your Name");
        bounds = xy::BitmapText::getLocalBounds(entity);
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

        //input text
        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(boxPos);
        entity.getComponent<xy::Transform>().setScale(2.f, 2.f);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::BitmapText>(font);// .setString("Sarah Connor");
        entity.addComponent<xy::CommandTarget>().ID = CommandID::ScoreInput;
        bounds = xy::BitmapText::getLocalBounds(entity);
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    }
}

void GameOverState::updatePlayerString()
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::ScoreInput;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<xy::BitmapText>().setString(m_playerName);
        auto bounds = xy::BitmapText::getLocalBounds(e);
        e.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}
