/*********************************************************************
(c) Matt Marchant 2019

This file is part of the xygine tutorial found at
https://github.com/fallahn/xygine

xygineXT - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "PauseState.hpp"
#include "ResourceIDs.hpp"
#include "GameConsts.hpp"

#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
    const sf::Time DelayTime = sf::seconds(0.5f); //delay input buy this so we don't accidentally close the menu
    const std::int32_t Deletable = 0x1;
    const std::int32_t ScoreString = 0x2;
    const std::int32_t TableString = 0x4;
    const std::size_t MaxInitials = 3;
}

PauseState::PauseState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_sharedData(sd),
    m_scoreShown(false)
{
    auto& mb = ctx.appInstance.getMessageBus();
    if (m_sharedData.pauseMessage == SharedData::GameOver)
    {
        m_scene.addSystem<xy::CommandSystem>(mb);

    }
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    //background
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-2); //makes sure it sits behind text
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts =
    {
        sf::Vertex(sf::Vector2f(0.f, 0.f), sf::Color(0,0,0,120)),
        sf::Vertex(sf::Vector2f(xy::DefaultSceneSize.x, 0.f), sf::Color(0,0,0,120)),
        sf::Vertex(xy::DefaultSceneSize, sf::Color(0,0,0,120)),
        sf::Vertex(sf::Vector2f(0.f, xy::DefaultSceneSize.y), sf::Color(0,0,0,120)),
    };
    entity.getComponent<xy::Drawable>().updateLocalBounds(); // this is required whenever vertex data is modified

    switch (sd.pauseMessage)
    {
    default:
        break;
    case SharedData::Paused:
        createPause();
        break;
    case SharedData::GameOver:
        createGameover();
        break;
    case SharedData::Died:
        createContinue();
        break;
    case SharedData::Error:
        createError();
        break;
    }

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    xy::App::setMouseCursorVisible(true);
}

//public
bool PauseState::handleEvent(const sf::Event& evt)
{
    if (m_delayClock.getElapsedTime() < DelayTime)
    {
        return false;
    }

    //oh what have I done? Take note, don't be lazy and try
    //cramming so many state types into one state class!!
    //TODO refactor this mess before it gets any more knotted

    if (evt.type == sf::Event::KeyReleased)
    {
        if (m_sharedData.pauseMessage == SharedData::Paused)
        {
            switch (evt.key.code)
            {
            default: break;
            case sf::Keyboard::P:
            case sf::Keyboard::Escape:
            case sf::Keyboard::Pause:
                requestStackPop();
                break;
            case sf::Keyboard::Q:
                requestStackClear();
                requestStackPush(StateID::MainMenu);
                break;
            }
        }
        else if (m_sharedData.pauseMessage == SharedData::Died)
        {
            requestStackPop();
        }
        else if (m_sharedData.pauseMessage == SharedData::GameOver)
        {
            //move to next level if game won
            if (m_sharedData.gameoverType == SharedData::Win)
            {
                LOG("Refactor this into own GameOver state", xy::Logger::Type::Info);
                m_sharedData.playerData.currentMap = (m_sharedData.playerData.currentMap + 1) % ConstVal::MapNames.size();
                requestStackClear();
                requestStackPush(StateID::Game);
            }
            else
            {
                showScoreInput();
            }
        }
        else if(m_sharedData.pauseMessage == SharedData::Error)
        {
            requestStackClear();
            requestStackPush(StateID::MainMenu);
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed
        && evt.joystickButton.joystickId == 0)
    {
        switch (evt.joystickButton.button)
        {
        default: break;
        case 6:
        case 1:
            if (m_sharedData.pauseMessage == SharedData::GameOver)
            {
                //move to next level if game won
                if (m_sharedData.gameoverType == SharedData::Win)
                {
                    LOG("Refactor this into own GameOver state", xy::Logger::Type::Info);
                    m_sharedData.playerData.currentMap = (m_sharedData.playerData.currentMap + 1) % ConstVal::MapNames.size();
                    requestStackClear();
                    requestStackPush(StateID::Game);
                }
                else
                {
                    showScoreInput();
                }
            }
            else if (m_sharedData.pauseMessage == SharedData::Error)
            {
                requestStackClear();
                requestStackPush(StateID::MainMenu);
            }
            else
            {
                requestStackPop();
            }
            break;
        case 0:
            if (m_sharedData.pauseMessage == SharedData::Died)
            {
                requestStackPop();
            }
            break;
        }
    }

    else if (evt.type == sf::Event::TextEntered
        && m_scoreShown)
    {
        if (m_initialsString.getSize() < MaxInitials
            && evt.text.unicode > 31)
        {
            m_initialsString += sf::String(evt.text.unicode);
            updateScoreString();
        }
    }
    else if (evt.type == sf::Event::KeyPressed
        && m_scoreShown)
    {
        if (m_initialsString.getSize() > 0
            && evt.key.code == sf::Keyboard::BackSpace)
        {
            m_initialsString.erase(m_initialsString.getSize() - 1);
            updateScoreString();
        }
        else if (evt.key.code == sf::Keyboard::Enter)
        {
            submitScore();
        }
    }

    m_scene.forwardEvent(evt);
    return false; //this is a pause state - don't forward events to states below!
}

void PauseState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool PauseState::update(float dt)
{
    static float shaderTime = 0.f;
    shaderTime += dt * 0.001f;
    m_sharedData.shaders->get(ShaderID::Cloud).setUniform("u_time", shaderTime);
    m_sharedData.shaders->get(ShaderID::Noise).setUniform("u_time", shaderTime);

    m_scene.update(dt);
    return false;
}

void PauseState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void PauseState::createPause()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);

    //text entities
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -160.f);
    entity.addComponent<xy::Text>(font).setString("PAUSED");
    entity.getComponent<xy::Text>().setCharacterSize(180);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 148.f);
    entity.addComponent<xy::Text>(font).setString("Press Q to Quit or P to Continue");
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
}

void PauseState::createContinue()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);

    //text entities
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -160.f);
    entity.addComponent<xy::Text>(font).setString("DRONE LOST");
    entity.getComponent<xy::Text>().setCharacterSize(180);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 148.f);
    entity.addComponent<xy::Text>(font).setString("Press Any Key To Continue");
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
}

void PauseState::createGameover()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);

    //text entities
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -160.f);
    entity.addComponent<xy::Text>(font).setString("GAME OVER");
    entity.getComponent<xy::Text>().setCharacterSize(180);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Deletable;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 128.f);
    if (m_sharedData.gameoverType == SharedData::Win)
    {
        entity.addComponent<xy::Text>(font).setString("You Win");
    }
    else
    {
        entity.addComponent<xy::Text>(font).setString("You Lose");
    }
    entity.getComponent<xy::Text>().setCharacterSize(50);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Deletable;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 348.f);
    entity.addComponent<xy::Text>(font).setString("Press Any Key");
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Deletable;
}

void PauseState::createError()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);

    //text entities
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -160.f);
    entity.addComponent<xy::Text>(font).setString("Error");
    entity.getComponent<xy::Text>().setCharacterSize(180);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 148.f);
    entity.addComponent<xy::Text>(font).setString(m_sharedData.messageString);
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
}

void PauseState::showScoreInput()
{
    if (m_scoreShown)
    {
        return;
    }

    xy::Command cmd;
    cmd.targetFlags = Deletable;
    cmd.action = [&](xy::Entity e, float)
    {
        m_scene.destroyEntity(e);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 348.f);
    entity.addComponent<xy::Text>(font).setString(">___");
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = ScoreString;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -148.f);
    entity.addComponent<xy::Text>(font).setString("Enter Your Initials");
    entity.getComponent<xy::Text>().setCharacterSize(60);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = TableString;

    m_scoreShown = true;
}

void PauseState::updateScoreString()
{
    xy::Command cmd;
    cmd.targetFlags = ScoreString;
    cmd.action = [&](xy::Entity e, float)
    {
        if (!m_initialsString.isEmpty())
        {
            e.getComponent<xy::Text>().setString(m_initialsString);
        }
        else
        {
            e.getComponent<xy::Text>().setString(">___");
        }
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void PauseState::submitScore()
{
    /*
    You know what? No. I'm not going to do this here.
    It can wait until I make a proper score entry state
    instead of trying to cram it into this misappropriated
    pause state.
    */

    if (m_initialsString.getSize() > MaxInitials)
    {
        //we have '>___' so replace it
        m_initialsString = "AAA";
    }

    //insert into scores
    //TODO use something sortable like xy::ConfigFile ?

    //set table string
    xy::Command cmd;
    cmd.targetFlags = ScoreString;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<xy::Text>().setString("Press any Key To Continue");
        e.getComponent<xy::CommandTarget>().ID = 0;
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = TableString;
    cmd.action = [](xy::Entity e, float)
    {
        sf::String str =
        {
            " 1. AAA      10000\n 2. BUN      9001\n 3. SFL      9000\n 4. APS      8000\n 5. WEE      7000\n 6. SLE      6000\n 7. BLO      5000\n 8. JOB      4000\n 9. POO      3000\n10. ASS      2000"
        };
        e.getComponent<xy::Text>().setString(str);
        e.getComponent<xy::Text>().setCharacterSize(40);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //fudge the state so key presses return to the main menu
    m_sharedData.pauseMessage = SharedData::Error;
    m_delayClock.restart();
}