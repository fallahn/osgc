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
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

#include <sstream>
#include <iomanip>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(1.f);

    const std::string noiseFrag = R"(
        #version 120

        uniform float u_time;

        float rand(vec2 pos)
        {
            return fract(sin(dot(pos, vec2(12.9898, 4.1414) + u_time)) * 43758.5453);
        }

        void main()
        {
            vec4 noise = vec4(vec3(rand(floor((gl_FragCoord.xy / 4.0)))), 1.0);

            gl_FragColor = noise;

        })";

    const sf::Time textUpdateTime = sf::seconds(0.05f);
    const std::int32_t coinScore = 100;
    const std::int32_t timeScore = 50;
}

SummaryState::SummaryState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_sharedData(sd),
    m_state     (AddCoins)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool SummaryState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    auto exec = [&]()
    {
        switch (m_state)
        {
        default: break;
        case AddCoins:
            m_sharedData.inventory.score += m_sharedData.inventory.coins * coinScore;
            m_sharedData.inventory.coins = 0;
            break;
        case AddTime:
            m_sharedData.inventory.score += m_sharedData.roundTime * timeScore;
            m_sharedData.roundTime = 0;
            break;
        case Completed:
            requestStackClear();
            if (m_sharedData.nextMap == "credits.tmx")
            {
                requestStackPush(StateID::Ending);
            }
            else
            {
                requestStackPush(StateID::Game); //TODO check shared data for game type
            }
            break;
        }
    };

    //slight delay before accepting input
    if (delayClock.getElapsedTime() > delayTime)
    {
        if (evt.type == sf::Event::KeyPressed)
        {
            exec();
        }
        else if (evt.type == sf::Event::JoystickButtonPressed
            && evt.joystickButton.joystickId == 0)
        {
            if (evt.joystickButton.button == 0)
            {
                exec();
            }
        }
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
    static float shaderTime = 0.f;
    shaderTime += dt;

    m_shader.setUniform("u_time", shaderTime);

    if (m_state == AddCoins)
    {
        addCoins();
    }
    else if (m_state == AddTime)
    {
        addTime();
    }

    m_scene.update(dt);
    return false;
}

void SummaryState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void SummaryState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    m_shader.loadFromMemory(noiseFrag, sf::Shader::Fragment);

    //background
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::Depth::Background);

    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.emplace_back();
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x, 0.f));
    verts.emplace_back(xy::DefaultSceneSize);
    verts.emplace_back(sf::Vector2f(0.f, xy::DefaultSceneSize.y));

    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.getComponent<xy::Drawable>().setShader(&m_shader);


    sf::Color c(0, 0, 0, 120);
    sf::Vector2f size(640.f, 480.f);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::Depth::Background + 1);

    auto& moreVerts = entity.getComponent<xy::Drawable>().getVertices();
    moreVerts.emplace_back(-size, c);
    moreVerts.emplace_back(sf::Vector2f(size.x, -size.y), c);
    moreVerts.emplace_back(size, c);
    moreVerts.emplace_back(sf::Vector2f(-size.x, size.y), c);

    entity.getComponent<xy::Drawable>().updateLocalBounds();

    //text
    auto fontID = m_resources.load<sf::Font>(FontID::GearBoyFont);
    auto& font = m_resources.get<sf::Font>(fontID);

    auto createText = [&]()->xy::Entity
    {
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::Depth::Text);
        entity.addComponent<xy::Text>(font).setCharacterSize(GameConst::UI::MediumTextSize);
        entity.getComponent<xy::Text>().setFillColour(sf::Color::White);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        return entity;
    };

    entity = createText();
    entity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::LargeTextSize);
    entity.getComponent<xy::Text>().setString("SCORE");
    entity.getComponent<xy::Transform>().move(0.f, -460.f);

    entity = createText();
    entity.getComponent<xy::Text>().setString(std::to_string(m_sharedData.inventory.score));
    entity.getComponent<xy::Transform>().move(0.f, -280.f);
    m_textEnts[Score] = entity;

    std::stringstream ss;
    ss << "COINS: " << std::setw(3) << std::setfill('0') << m_sharedData.inventory.coins;

    entity = createText();
    entity.getComponent<xy::Text>().setString(ss.str());
    entity.getComponent<xy::Transform>().move(0.f, -60.f);
    m_textEnts[Coins] = entity;

    std::stringstream sd;
    sd << "TIME: " << std::setw(3) << std::setfill('0') << m_sharedData.roundTime;

    entity = createText();
    entity.getComponent<xy::Text>().setString(sd.str());
    entity.getComponent<xy::Transform>().move(0.f, 60.f);
    m_textEnts[Time] = entity;

    entity = createText();
    entity.getComponent<xy::Text>().setString("Press any key");
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Transparent);
    entity.getComponent<xy::Transform>().move(0.f, 380.f);
    m_textEnts[OK] = entity;
}

void SummaryState::addCoins()
{
    std::stringstream ss;
    ss << "COINS: " << std::setw(3) << std::setfill('0') << m_sharedData.inventory.coins;
    m_textEnts[Coins].getComponent<xy::Text>().setString(ss.str());
    m_textEnts[Score].getComponent<xy::Text>().setString(std::to_string(m_sharedData.inventory.score));

    if (m_sharedData.inventory.coins > 0)
    {
        if (m_stateClock.getElapsedTime() > textUpdateTime)
        {
            m_sharedData.inventory.coins--;
            m_sharedData.inventory.score += coinScore;
            m_stateClock.restart();
        }
    }
    else
    {
        m_stateClock.restart();
        m_state = AddTime;
    }
}

void SummaryState::addTime()
{
    std::stringstream sd;
    sd << "TIME: " << std::setw(3) << std::setfill('0') << m_sharedData.roundTime;
    m_textEnts[Time].getComponent<xy::Text>().setString(sd.str());
    m_textEnts[Score].getComponent<xy::Text>().setString(std::to_string(m_sharedData.inventory.score));

    if (m_sharedData.roundTime > 0)
    {
        if (m_stateClock.getElapsedTime() > textUpdateTime)
        {
            m_sharedData.roundTime--;
            m_sharedData.inventory.score += timeScore;
            m_stateClock.restart();
        }
    }
    else
    {
        m_textEnts[OK].getComponent<xy::Text>().setFillColour(sf::Color::White);
        m_state = Completed;
        m_sharedData.saveProgress();
    }
}
