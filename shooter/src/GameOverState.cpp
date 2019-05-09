/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

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

#include "GameOverState.hpp"
#include "StateIDs.hpp"
#include "ResourceIDs.hpp"
#include "GameConsts.hpp"
#include "MenuConsts.hpp"

#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

namespace
{
    const std::int32_t Deletable = 0x1;
    const std::int32_t ScoreString = 0x2;
    const std::int32_t TableString = 0x4;
    const std::size_t MaxInitials = 3;

    sf::Time DelayTime = sf::seconds(0.5f);
}

GameOverState::GameOverState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_scoreShown(false)
{
    auto& mb = ctx.appInstance.getMessageBus();

    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    //background
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-2); //makes sure it sits behind text
    auto & verts = entity.getComponent<xy::Drawable>().getVertices();
    verts =
    {
        sf::Vertex(sf::Vector2f(0.f, 0.f), sf::Color(0,0,0,ConstVal::OverlayTransparency)),
        sf::Vertex(sf::Vector2f(xy::DefaultSceneSize.x, 0.f), sf::Color(0,0,0,ConstVal::OverlayTransparency)),
        sf::Vertex(xy::DefaultSceneSize, sf::Color(0,0,0,ConstVal::OverlayTransparency)),
        sf::Vertex(sf::Vector2f(0.f, xy::DefaultSceneSize.y), sf::Color(0,0,0,ConstVal::OverlayTransparency)),
    };
    entity.getComponent<xy::Drawable>().updateLocalBounds();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    createGameover();

    xy::App::setMouseCursorVisible(true);
}

//public
bool GameOverState::handleEvent(const sf::Event& evt)
{
    if (m_delayClock.getElapsedTime() < DelayTime)
    {
        return false;
    }

	//prevents events being forwarded if the console wishes to consume them
	if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        //this is a fudge set by submitscore() - we'll get rid of this eventually
        if (m_sharedData.pauseMessage == SharedData::None)
        {
            requestStackClear();
            requestStackPush(StateID::MainMenu);
        }

        else
        {
            //move to next level if game won
            if (m_sharedData.gameoverType == SharedData::Win)
            {
                gotoNextMap();
            }
            else
            {
                showScoreInput();
            }
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed
        && evt.joystickButton.joystickId == 0)
    {
        //move to next level if game won
        if (m_sharedData.gameoverType == SharedData::Win)
        {
            gotoNextMap();
        }
        else
        {
            showScoreInput();
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

    return false;
}

void GameOverState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool GameOverState::update(float dt)
{
    static float shaderTime = 0.f;
    shaderTime += dt * 0.001f;
    m_sharedData.shaders->get(ShaderID::Cloud).setUniform("u_time", shaderTime);
    m_sharedData.shaders->get(ShaderID::Noise).setUniform("u_time", shaderTime);

    m_scene.update(dt);
    return false;
}

void GameOverState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

xy::StateID GameOverState::stateID() const
{
    return StateID::GameOver;
}

//private
void GameOverState::createGameover()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);

    std::string mainString;
    std::string subString;
    std::uint32_t textSize = 176; //best size is multiple of 8

    if (m_sharedData.gameoverType == SharedData::Win)
    {
        mainString = "Round Complete";
        subString = std::to_string(m_sharedData.playerData.colonistsSaved) + " Colonists Were Saved!";
        textSize = 120;
    }
    else
    {
        mainString = "GAME OVER";
        subString = (m_sharedData.playerData.lives == 0) ? "No Drones Left" : "The Colonists All Died";
    }

    //text entities
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -180.f);
    entity.addComponent<xy::Text>(font).setString(mainString);
    entity.getComponent<xy::Text>().setCharacterSize(textSize);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Deletable;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 28.f);
    entity.addComponent<xy::Text>(font).setString(subString);
    entity.getComponent<xy::Text>().setCharacterSize(48);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Deletable;

    if (m_sharedData.gameoverType == SharedData::Win)
    {
        std::int32_t bonus = m_sharedData.playerData.colonistsSaved * 100;
        m_sharedData.playerData.score += bonus;

        entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().move(0.f, 130.f);
        entity.addComponent<xy::Text>(font).setString("BONUS 100 x " + std::to_string(m_sharedData.playerData.colonistsSaved));
        entity.getComponent<xy::Text>().setCharacterSize(48);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        entity.getComponent<xy::Text>().setFillColour(sf::Color::Yellow);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::CommandTarget>().ID = Deletable;
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().userData = std::make_any<float>(0.5f);
        entity.getComponent<xy::Callback>().function =
            [](xy::Entity e, float dt)
        {
            auto& currTime = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
            currTime -= dt;

            if (currTime < 0)
            {
                currTime = 0.5f;

                auto colour = e.getComponent<xy::Text>().getFillColour();
                colour.a = (colour.a == 0) ? 255 : 0;
                e.getComponent<xy::Text>().setFillColour(colour);
            }
        };
    }

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, 348.f);
    entity.addComponent<xy::Text>(font).setString("Press Any Key");
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = Deletable;
}

void GameOverState::gotoNextMap()
{
    m_sharedData.playerData.currentMap = (m_sharedData.playerData.currentMap + 1) % m_sharedData.mapNames.size();
    requestStackClear();
    requestStackPush(StateID::Game);
}

void GameOverState::showScoreInput()
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
    entity.getComponent<xy::Transform>().move(0.f, 148.f);
    entity.addComponent<xy::Text>(font).setString(">___");
    entity.getComponent<xy::Text>().setCharacterSize(40);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = ScoreString;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.getComponent<xy::Transform>().move(0.f, -148.f);
    entity.addComponent<xy::Text>(font).setString("Enter Your Initials");
    entity.getComponent<xy::Text>().setCharacterSize(64);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = TableString;

    m_scoreShown = true;
}

void GameOverState::updateScoreString()
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

void GameOverState::submitScore()
{
    /*
    This was pulled from the mess of the overloaded pause state
    Eventually needs to be redone....
    */

    if (m_initialsString.getSize() > MaxInitials)
    {
        //we have '>___' so replace it
        m_initialsString = "AAA";
    }

    std::string tableStr;
    auto setString = [&](std::vector<xy::ConfigProperty> properties)
    {
        if (properties.empty())
        {
            tableStr = "No Scores Yet...";
        }
        else
        {
            std::sort(properties.begin(), properties.end(), [](const xy::ConfigProperty& a, const xy::ConfigProperty& b)
                {
                    return a.getValue<std::int32_t>() > b.getValue<std::int32_t>();
                });

            auto count = std::min(std::size_t(10), properties.size());
            for (auto i = 0u; i < count; ++i)
            {
                auto name = properties[i].getName();
                tableStr += name.substr(0, std::min(std::size_t(3), name.size())) + "      " + properties[i].getValue<std::string>() + "\n";
            }
        }
    };

    //insert into scores
    std::string initials = m_initialsString.toAnsiString();
    std::replace(initials.begin(), initials.end(), ' ', '_');
    std::transform(initials.begin(), initials.end(), initials.begin(), ::toupper);

    if (m_sharedData.difficulty == SharedData::Easy)
    {
        m_sharedData.highScores.easy.addProperty(initials, std::to_string(m_sharedData.playerData.score));
        setString(m_sharedData.highScores.easy.getProperties());
        m_sharedData.highScores.easy.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreEasyName);
    }
    else if (m_sharedData.difficulty == SharedData::Medium)
    {
        m_sharedData.highScores.medium.addProperty(initials, std::to_string(m_sharedData.playerData.score));
        setString(m_sharedData.highScores.medium.getProperties());
        m_sharedData.highScores.medium.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreMedName);
    }
    else
    {
        m_sharedData.highScores.hard.addProperty(initials, std::to_string(m_sharedData.playerData.score));
        setString(m_sharedData.highScores.hard.getProperties());
        m_sharedData.highScores.hard.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreHardName);
    }

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
    cmd.action = [tableStr](xy::Entity e, float)
    {
        e.getComponent<xy::Text>().setString(tableStr);
        e.getComponent<xy::Text>().setCharacterSize(40);
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //fudge the state so key presses return to the main menu
    m_sharedData.pauseMessage = SharedData::None;
    m_scoreShown = false; //prevents 'any key' submitting score again if it is return or enter
    m_delayClock.restart();
}