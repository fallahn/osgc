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
#include "GameConsts.hpp"
#include "StateIDs.hpp"
#include "ResourceIDs.hpp"
#include "PluginExport.hpp"
#include "SliderSystem.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "KeyMapping.hpp"
#include "Drone.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/core/FileSystem.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

#include <fstream>

namespace
{
    const std::string TextFrag =
        R"(
    #version 120

    uniform sampler2D u_texture;
    uniform float u_alpha;

    void main()
    {
        vec2 coord = gl_TexCoord[0].xy;
        coord.x *= (1.0 + coord.y);
        coord.x -= (0.5 * coord.y);

        gl_FragColor = texture2D(u_texture, coord) * gl_Color * u_alpha;
    })";

    class FlashCallback final
    {
    public:
        void operator() (xy::Entity e, float dt)
        {
            m_currentTime -= dt;
            if (m_currentTime < 0)
            {
                m_currentTime = 0.25f;

                auto colour = e.getComponent<xy::Text>().getFillColour();
                colour.a = (colour.a == 0) ? 255 : 0;
                e.getComponent<xy::Text>().setFillColour(colour);
             }
        }

    private:
        float m_currentTime = 0.25f;
    };
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_scene         (ctx.appInstance.getMessageBus()),
    m_textCrawl     (sd.resources.get<sf::Font>(FontID::handles[FontID::CGA])),
    m_menuActive    (false),
    m_highScores    (3),
    m_scoreIndex    (0)
{
    launchLoadingScreen();

    m_activeMapping.joyButtonDest = nullptr;
    m_scoreTitles = { "Novice", "Seasoned", "Pro" };

    initScene();
    loadAssets();
    buildMenu();
    buildStarfield();
    buildHelp();
    buildDifficultySelect();
    buildHighScores();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    xy::App::setMouseCursorVisible(true);

    registerConsoleTab("About", 
        []()
        {
            xy::Nim::text("Drone Drop (c)2019 Matt Marchant and Contributors");
            xy::Nim::text("For individual asset credits and licensing see credits.txt in the \'assets\' directory");
        });

    quitLoadingScreen();
}

MenuState::~MenuState()
{
    saveSettings();
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    if (m_menuActive)
    {
        m_scene.getSystem<xy::UISystem>().handleEvent(evt);
    }
    else
    {
        if (evt.type == sf::Event::KeyReleased
            || evt.type == sf::Event::JoystickButtonPressed
            || evt.type == sf::Event::MouseButtonReleased)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Menu::RootNode | CommandID::Starfield;
            cmd.action =
                [](xy::Entity entity, float)
            {
                auto& slider = entity.getComponent<Slider>();
                if (!slider.active)
                {
                    //slider.target = {}; //target was set on entity creation
                    slider.active = true;
                }
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = CommandID::Menu::TextCrawl;
            cmd.action =
                [](xy::Entity entity, float)
            {
                entity.getComponent<xy::Callback>().active = true;
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }

    if (m_activeMapping.joybindActive)
    {
        if (evt.type == sf::Event::JoystickButtonPressed
            && evt.joystickButton.joystickId == 0)
        {
            //TODO check existing bindings (but not the requested one)
            //to see if key code already bound

            *m_activeMapping.joyButtonDest = evt.joystickButton.button;
            m_activeMapping.joyButtonDest = nullptr;
            m_activeMapping.joybindActive = false;

            auto ent = m_activeMapping.displayEntity;
            m_activeMapping.displayEntity = {};

            ent.getComponent<xy::Text>().setString(std::to_string(evt.joystickButton.button));
            auto colour = ent.getComponent<xy::Text>().getFillColour();
            colour.a = 255;
            ent.getComponent<xy::Text>().setFillColour(colour);
            ent.getComponent<xy::Callback>().active = false;
        }
    }
    else if (m_activeMapping.keybindActive)
    {
        if (evt.type == sf::Event::KeyReleased)
        {
            //TODO check existing bindings (but not the requested one)
            //to see if key code already bound

            *m_activeMapping.keyDest = evt.key.code;
            m_activeMapping.keyDest = nullptr;
            m_activeMapping.keybindActive = false;

            auto ent = m_activeMapping.displayEntity;
            m_activeMapping.displayEntity = {};

            ent.getComponent<xy::Text>().setString(KeyMapping.at(evt.key.code));
            auto colour = ent.getComponent<xy::Text>().getFillColour();
            colour.a = 255;
            ent.getComponent<xy::Text>().setFillColour(colour);
            ent.getComponent<xy::Callback>().active = false;
        }
    }


    m_scene.forwardEvent(evt);
    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::MenuMessage)
    {
        const auto& data = msg.getData<MenuEvent>();
        if (data.action == MenuEvent::SlideFinished)
        {
            m_menuActive = true;
        }
    }

    m_scene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
    if (m_textCrawl.update(dt))
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::Menu::RootNode | CommandID::Starfield;
        cmd.action =
            [](xy::Entity entity, float)
        {
            auto& slider = entity.getComponent<Slider>();
            if (!slider.active)
            {
                //slider.target = {}; //target was set on entity creation
                slider.active = true;
            }
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        //tidies up finished text entities
        cmd.targetFlags = CommandID::Menu::TextCrawl;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<xy::Callback>().active = true;
        };
        m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }

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
    if (!m_configSettings.loadFromFile(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ConfigName))
    {
        m_configSettings.addProperty("up", std::to_string(m_sharedData.keymap.up));
        m_configSettings.addProperty("down", std::to_string(m_sharedData.keymap.down));
        m_configSettings.addProperty("left", std::to_string(m_sharedData.keymap.left));
        m_configSettings.addProperty("right", std::to_string(m_sharedData.keymap.right));
        m_configSettings.addProperty("fire", std::to_string(m_sharedData.keymap.fire));
        m_configSettings.addProperty("pickup", std::to_string(m_sharedData.keymap.pickup));
        m_configSettings.addProperty("joy_fire", std::to_string(m_sharedData.keymap.joyFire));
        m_configSettings.addProperty("joy_pickup", std::to_string(m_sharedData.keymap.joyPickup));
        m_configSettings.addProperty("difficulty", std::to_string(m_sharedData.difficulty));
        m_configSettings.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ConfigName);
    }

    //validate loaded settings incase someone dicked around with the file
    if (!m_configSettings.findProperty("up")) { m_configSettings.addProperty("up", std::to_string(m_sharedData.keymap.up)); }
    if (!m_configSettings.findProperty("down")) { m_configSettings.addProperty("down", std::to_string(m_sharedData.keymap.down)); }
    if (!m_configSettings.findProperty("left")) { m_configSettings.addProperty("left", std::to_string(m_sharedData.keymap.left)); }
    if (!m_configSettings.findProperty("right")) { m_configSettings.addProperty("right", std::to_string(m_sharedData.keymap.right)); }
    if (!m_configSettings.findProperty("fire")) { m_configSettings.addProperty("fire", std::to_string(m_sharedData.keymap.fire)); }
    if (!m_configSettings.findProperty("pickup")) { m_configSettings.addProperty("pickup", std::to_string(m_sharedData.keymap.pickup)); }
    if (!m_configSettings.findProperty("joy_fire")) { m_configSettings.addProperty("joy_fire", std::to_string(m_sharedData.keymap.joyFire)); }
    if (!m_configSettings.findProperty("joy_pickup")) { m_configSettings.addProperty("joy_pickup", std::to_string(m_sharedData.keymap.joyPickup)); }
    if (!m_configSettings.findProperty("difficulty")) { m_configSettings.addProperty("difficulty", std::to_string(m_sharedData.difficulty)); }

    //then read back into the struct
    m_sharedData.keymap.up = static_cast<sf::Keyboard::Key>(m_configSettings.findProperty("up")->getValue<std::int32_t>());
    m_sharedData.keymap.down = static_cast<sf::Keyboard::Key>(m_configSettings.findProperty("down")->getValue<std::int32_t>());
    m_sharedData.keymap.left = static_cast<sf::Keyboard::Key>(m_configSettings.findProperty("left")->getValue<std::int32_t>());
    m_sharedData.keymap.right = static_cast<sf::Keyboard::Key>(m_configSettings.findProperty("right")->getValue<std::int32_t>());
    m_sharedData.keymap.fire = static_cast<sf::Keyboard::Key>(m_configSettings.findProperty("fire")->getValue<std::int32_t>());
    m_sharedData.keymap.pickup = static_cast<sf::Keyboard::Key>(m_configSettings.findProperty("pickup")->getValue<std::int32_t>());
    m_sharedData.keymap.joyFire = m_configSettings.findProperty("joy_fire")->getValue<std::int32_t>();
    m_sharedData.keymap.joyPickup = m_configSettings.findProperty("joy_pickup")->getValue<std::int32_t>();
    m_sharedData.difficulty = m_configSettings.findProperty("difficulty")->getValue<std::int32_t>();

    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<SliderSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
    m_scene.addSystem<xy::AudioSystem>(mb);
}

void MenuState::loadAssets()
{
    m_crawlShader.loadFromMemory(TextFrag, sf::Shader::Fragment);
    m_crawlShader.setUniform("u_alpha", 1.f);

    TextureID::handles[TextureID::MenuBackground] = m_resources.load<sf::Texture>("assets/images/menu_background.png");
    TextureID::handles[TextureID::HowToPlay] = m_resources.load<sf::Texture>("assets/images/how_to_play.png");
    TextureID::handles[TextureID::DifficultySelect] = m_resources.load<sf::Texture>("assets/images/difficulty_select.png");
    TextureID::handles[TextureID::HighScores] = m_resources.load<sf::Texture>("assets/images/high_scores.png");

    m_sharedData.mapNames.clear();
    std::ifstream file(xy::FileSystem::getResourcePath() + "assets/maps/mapcycle.txt");
    if (file.is_open() && file.good())
    {
        //read in map names
        std::string line;
        while (std::getline(file, line))
        {
            if (xy::FileSystem::getFileExtension(line) == ".tmx")
            {
                m_sharedData.mapNames.push_back(line);
            }
        }

        if (m_sharedData.mapNames.empty())
        {
            xy::Logger::log("No valid maps were loaded from mapcycle.txt", xy::Logger::Type::Error);

            requestStackClear();
            requestStackPush(StateID::ParentState);
        }
    }
    else
    {
        //safest thing to do is quit plugin
        xy::Logger::log("Failed to open mapcycle.txt", xy::Logger::Type::Error);

        requestStackClear();
        requestStackPush(StateID::ParentState);
    }
}

void MenuState::buildMenu()
{
    //root node for sliding
    auto rootNode = m_scene.createEntity();
    rootNode.addComponent<xy::Transform>().setPosition(0.f, Menu::PlanetHiddenPosition);
    rootNode.addComponent<Slider>().speed = 1.f;
    rootNode.addComponent<xy::CommandTarget>().ID = CommandID::Menu::RootNode;
    
    //title
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Menu::TitlePosition);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Drone Drop!");
    entity.getComponent<xy::Text>().setCharacterSize(120);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //menu items
    auto itemPos = Menu::ItemFirstPosition;
    //play
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Start Game");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);

    auto textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::LeftMouse)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Menu::DifficultySelect;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<Slider>().target = Menu::DifficultyShownPosition;
                e.getComponent<Slider>().active = true;
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = CommandID::Menu::RootNode;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<Slider>().target.y = Menu::PlanetHiddenPosition;
                e.getComponent<Slider>().active = true;
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            cmd.targetFlags = CommandID::Menu::Starfield;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<Slider>().target.y = Menu::StarfieldUpPosition;
                e.getComponent<Slider>().active = true;
            };
            m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    });
    itemPos.y += Menu::ItemVerticalSpacing;
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //options
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Options");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);

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
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //how to play / controls
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Help/Controls");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);

    textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::Help;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = Menu::HelpShownPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::PlanetHiddenPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::Starfield;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::StarfieldUpPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    itemPos.y += Menu::ItemVerticalSpacing;
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //high scores
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("High Scores");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);

    textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::ScoreMenu;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target = xy::DefaultSceneSize / 2.f;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::PlanetHiddenPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::Starfield;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::StarfieldUpPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    itemPos.y += Menu::ItemVerticalSpacing;
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //return to launcher
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Return to Launcher");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);

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
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //quit
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPos);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Quit To Desktop");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);

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
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    //intro screen
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, -1000.f);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Press Any Button To Skip");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize / 2);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::TextCrawl;
    entity.addComponent<xy::Callback>().userData = std::make_any<float>(1.f);
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float dt)
    {
        auto& currTime = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        currTime = std::max(0.f, currTime - dt);

        auto alpha = static_cast<sf::Uint8>(255.f * currTime);
        e.getComponent<xy::Text>().setFillColour({ 255,255,0,alpha });

        if (currTime == 0)
        {
            m_scene.destroyEntity(e);
        }
    };
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, -xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>().setDepth(Menu::TextRenderDepth);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.resize(6);
    verts[0] = { sf::Vector2f(0.f, 220.f), sf::Color(0,0,0,120) };
    verts[1] = { sf::Vector2f(xy::DefaultSceneSize.x, 220.f), sf::Color(0,0,0,120), sf::Vector2f(960.f, 0.f) };
    verts[2] = { sf::Vector2f(0.f, 420.f), sf::Vector2f(0.f, xy::DefaultSceneSize.y / 8.f) };
    verts[3] = { sf::Vector2f(xy::DefaultSceneSize.x, 420.f), sf::Vector2f(960.f, xy::DefaultSceneSize.y / 8.f) };
    verts[4] = { sf::Vector2f(0.f, xy::DefaultSceneSize.y - 60.f), sf::Vector2f(0.f, xy::DefaultSceneSize.y / 2.f)};
    verts[5] = { sf::Vector2f(xy::DefaultSceneSize.x, xy::DefaultSceneSize.y - 60.f), xy::DefaultSceneSize / 2.f };

    entity.getComponent<xy::Drawable>().setPrimitiveType(sf::TriangleStrip);
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.getComponent<xy::Drawable>().setTexture(&m_textCrawl.getTexture());
    entity.getComponent<xy::Drawable>().setShader(&m_crawlShader);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::TextCrawl;
    entity.addComponent<xy::Callback>().userData = std::make_any<float>(1.f);
    entity.getComponent<xy::Callback>().function =
        [&](xy::Entity e, float dt)
    {
        auto& currTime = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        currTime = std::max(0.f, currTime - dt);

        m_crawlShader.setUniform("u_alpha", currTime);

        if (currTime == 0)
        {
            m_scene.destroyEntity(e);
        }
    };

    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    //planet sprite
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::BackgroundRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::MenuBackground]));
    entity.addComponent<xy::AudioEmitter>().setSource("assets/sound/menu.ogg");
    entity.getComponent<xy::AudioEmitter>().setChannel(0);
    entity.getComponent<xy::AudioEmitter>().setAttenuation(0.f);
    entity.getComponent<xy::AudioEmitter>().setLooped(true);
    entity.getComponent<xy::AudioEmitter>().setVolume(0.3f);
    entity.getComponent<xy::AudioEmitter>().play();
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}

void MenuState::buildStarfield()
{
    auto addQuad = [](std::vector<sf::Vertex>& verts, sf::Vector2f position)
    {
        static const std::array<sf::Vector2f, 13u> sizes =
        {
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(6.f, 6.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(8.f, 8.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
            sf::Vector2f(4.f, 4.f),
        };

        auto quadSize = sizes[xy::Util::Random::value(0, 12)];

        verts.emplace_back(sf::Vector2f(position - (quadSize / 2.f)));
        verts.emplace_back(sf::Vector2f(position.x + (quadSize.x / 2.f), position.y - (quadSize.y / 2.f)));
        verts.emplace_back(sf::Vector2f(position + (quadSize / 2.f)));
        verts.emplace_back(sf::Vector2f(position.x - (quadSize.x / 2.f), position.y + (quadSize.y / 2.f)));
    };

    auto positions = xy::Util::Random::poissonDiscDistribution({ sf::Vector2f(), xy::DefaultSceneSize }, 100.f, 50);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(Menu::BackgroundRenderDepth - 2);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.clear();

    for (auto p : positions)
    {
        addQuad(verts, p);
    }

    entity.getComponent<xy::Drawable>().updateLocalBounds();

    //nearer stars to give depth
    positions = xy::Util::Random::poissonDiscDistribution({ sf::Vector2f(), sf::Vector2f(xy::DefaultSceneSize.x - 80.f, xy::DefaultSceneSize.y * 1.5f) }, 100.f, 50);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(40.f, 0.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::BackgroundRenderDepth - 2);
    auto& verts2 = entity.getComponent<xy::Drawable>().getVertices();
    verts2.clear();

    for (auto p : positions)
    {
        addQuad(verts2, p);
    }
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.addComponent<Slider>().speed = 1.f;
    entity.getComponent<Slider>().target = { 40.f, -xy::DefaultSceneSize.y / 16.f };
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::Starfield;

    //nearest
    positions = xy::Util::Random::poissonDiscDistribution({ sf::Vector2f(), sf::Vector2f(xy::DefaultSceneSize.x + 80.f, xy::DefaultSceneSize.y * 1.5f) }, 160.f, 40);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(-40.f, 0.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::BackgroundRenderDepth - 1);
    auto& verts3 = entity.getComponent<xy::Drawable>().getVertices();
    verts3.clear();

    for (auto p : positions)
    {
        addQuad(verts3, p);
    }
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.addComponent<Slider>().speed = 1.f;
    entity.getComponent<Slider>().target = { -40.f, Menu::StarfieldDownPosition };
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::Starfield;
}

void MenuState::buildHelp()
{
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(20.f, Menu::HelpHiddenPosition);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::HowToPlay]));
    entity.addComponent<Slider>().speed = 4.f;
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::Help;

    auto& parentTx = entity.getComponent<xy::Transform>();
    auto& uiSystem = m_scene.getSystem<xy::UISystem>();

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(256.f, 221.f);
    entity.addComponent<xy::UIHitBox>().area = { 0.f, 0.f, 45.f, 25.f };
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags) 
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::Help;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::HelpHiddenPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::PlanetShownPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::Starfield;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::StarfieldDownPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    saveSettings();

                    m_activeMapping.joybindActive = false;
                    m_activeMapping.keybindActive = false;
                    m_activeMapping.keyDest = nullptr;

                    auto ent = m_activeMapping.displayEntity;
                    m_activeMapping.displayEntity = {};

                    if (ent.isValid())
                    {
                        auto colour = ent.getComponent<xy::Text>().getFillColour();
                        colour.a = 255;
                        ent.getComponent<xy::Text>().setFillColour(colour);
                        ent.getComponent<xy::Callback>().active = false;
                    }
                }
            });
    parentTx.addChild(entity.getComponent<xy::Transform>());

    sf::Vector2f textPos(417.f, 39.f);
    const sf::Vector2f textScale(0.25f, 0.25f);
    const float verticalSpacing = 24.f;
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);
    
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(KeyMapping.at(m_sharedData.keymap.up));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    sf::FloatRect buttonArea(0.f, 0.f, 12.f, 12.f);
    auto buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(32.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeMapping.keybindActive = true;
                    m_activeMapping.keyDest = &m_sharedData.keymap.up;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(KeyMapping.at(m_sharedData.keymap.down));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(32.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeMapping.keybindActive = true;
                    m_activeMapping.keyDest = &m_sharedData.keymap.down;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(KeyMapping.at(m_sharedData.keymap.left));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(32.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeMapping.keybindActive = true;
                    m_activeMapping.keyDest = &m_sharedData.keymap.left;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(KeyMapping.at(m_sharedData.keymap.right));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(32.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeMapping.keybindActive = true;
                    m_activeMapping.keyDest = &m_sharedData.keymap.right;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(KeyMapping.at(m_sharedData.keymap.fire));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(32.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeMapping.keybindActive = true;
                    m_activeMapping.keyDest = &m_sharedData.keymap.fire;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(KeyMapping.at(m_sharedData.keymap.pickup));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(32.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_activeMapping.keybindActive = true;
                    m_activeMapping.keyDest = &m_sharedData.keymap.pickup;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());

    //joy buttons
    textPos.x += 17.f;
    textPos.y += 32.f;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(std::to_string(m_sharedData.keymap.joyPickup));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(15.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse
                    && sf::Joystick::isConnected(0))
                {
                    m_activeMapping.joybindActive = true;
                    m_activeMapping.joyButtonDest = &m_sharedData.keymap.joyPickup;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setString(std::to_string(m_sharedData.keymap.joyFire));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().function = FlashCallback();
    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonEnt = m_scene.createEntity();
    buttonEnt.addComponent<xy::Transform>().setPosition(textPos);
    buttonEnt.getComponent<xy::Transform>().move(15.f, -2.f);
    buttonEnt.addComponent<xy::UIHitBox>().area = buttonArea;
    buttonEnt.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&, entity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse
                    && sf::Joystick::isConnected(0))
                {
                    m_activeMapping.joybindActive = true;
                    m_activeMapping.joyButtonDest = &m_sharedData.keymap.joyFire;
                    m_activeMapping.displayEntity = entity;

                    entity.getComponent<xy::Callback>().active = true;
                }
            });
    parentTx.addChild(buttonEnt.getComponent<xy::Transform>());
}

void MenuState::buildDifficultySelect()
{
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::DifficultySelect]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, xy::DefaultSceneSize.y + (bounds.height * 2.f));
    entity.addComponent<xy::CommandTarget>().ID = CommandID::DifficultySelect;
    entity.addComponent<Slider>().speed = 4.f;
    entity.addComponent<xy::UIHitBox>().area = { 68.f, 85.f, 68.f, 17.f };
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&, bounds](xy::Entity, sf::Uint64 flags) 
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::DifficultySelect;
                    cmd.action = [bounds](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = xy::DefaultSceneSize.y + (bounds.height * 2.f);
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::PlanetShownPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::Starfield;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::StarfieldDownPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });


    auto& parentTx = entity.getComponent<xy::Transform>();
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);
    auto& uiSystem = m_scene.getSystem<xy::UISystem>();

    //add buttons
    sf::Vector2f buttonPos(bounds.width / 2.f, 30.f);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(buttonPos);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setString("Novice");
    bounds = xy::Text::getLocalBounds(entity);

    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.difficulty = SharedData::Easy;
                    m_sharedData.playerData = {};
                    requestStackClear();
                    requestStackPush(StateID::Game);
                }
            });
    parentTx.addChild(entity.getComponent<xy::Transform>());
    buttonPos.y += (bounds.height / 4.f) + 11.f;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(buttonPos);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setString("Seasoned");
    bounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.difficulty = SharedData::Medium;
                    m_sharedData.playerData = {};
                    requestStackClear();
                    requestStackPush(StateID::Game);
                }
            });
    parentTx.addChild(entity.getComponent<xy::Transform>());
    buttonPos.y += (bounds.height / 4.f) + 11.f;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(buttonPos);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(32);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Text>().setString("Pro");
    bounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_sharedData.difficulty = SharedData::Hard;
                    m_sharedData.playerData = {};
                    requestStackClear();
                    requestStackPush(StateID::Game);
                }
            });
    parentTx.addChild(entity.getComponent<xy::Transform>());

    //and some cheeky little animations
    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/science_large.spt", m_resources);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(12.f, 28.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("science");
    entity.addComponent<xy::SpriteAnimation>().play(0);
    parentTx.addChild(entity.getComponent<xy::Transform>());

    spriteSheet.loadFromFile("assets/sprites/science_small.spt", m_resources);

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(144.f, 28.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("science");
    entity.addComponent<xy::SpriteAnimation>().play(0);
    parentTx.addChild(entity.getComponent<xy::Transform>());
}

void MenuState::buildHighScores()
{
    auto setString = [&](std::size_t idx, std::vector<xy::ConfigProperty>& properties)
    {
        if (properties.empty())
        {
            m_highScores[idx] = "No Scores Yet...";
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
                m_highScores[idx] += name.substr(0, std::min(std::size_t(3), name.size())) + "      " + properties[i].getValue<std::string>() + "\n";
            }
        }
    };
    
    if (!m_sharedData.highScores.easy.loadFromFile(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreEasyName))
    {
        m_sharedData.highScores.easy.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreEasyName);
    }

    auto properties = m_sharedData.highScores.easy.getProperties();
    setString(0, properties);

    if (!m_sharedData.highScores.medium.loadFromFile(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreMedName))
    {
        m_sharedData.highScores.medium.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreMedName);
    }
    properties = m_sharedData.highScores.medium.getProperties();
    setString(1, properties);

    if (!m_sharedData.highScores.hard.loadFromFile(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreHardName))
    {
        m_sharedData.highScores.hard.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ScoreHardName);
    }
    properties = m_sharedData.highScores.hard.getProperties();
    setString(2, properties);

    //background
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::HighScores]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, Menu::ScoreHiddenPosition);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::ScoreMenu;
    entity.addComponent<Slider>().speed = 4.f;
    auto& parentTx = entity.getComponent<xy::Transform>();

    //score test
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(32);
    entity.getComponent<xy::Text>().setString(m_highScores[0]);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Transform>().setPosition(bounds.width / 2.f, 28.f);
    parentTx.addChild(entity.getComponent<xy::Transform>());
    auto scoreTextEntity = entity; //store this so we can copy into button lambdas

    //title text
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Text>(font).setCharacterSize(32);
    entity.getComponent<xy::Text>().setString(m_scoreTitles[0]);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.getComponent<xy::Transform>().setPosition(bounds.width / 2.f, 6.f);
    parentTx.addChild(entity.getComponent<xy::Transform>());
    auto titleTextEntity = entity;

    //wiggler
    /*xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/bar_anim.spt", m_resources);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(14.f, 123.f);
    entity.addComponent<xy::Drawable>().setDepth(Menu::MenuRenderDepth + Menu::TextRenderDepth);
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("anim");
    entity.addComponent<xy::SpriteAnimation>().play(0);
    parentTx.addChild(entity.getComponent<xy::Transform>());*/

    //button boxes
    sf::FloatRect buttonSize(0.f, 0.f, 16.f, 16.f);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(18.f, 158.f);
    entity.addComponent<xy::UIHitBox>().area = buttonSize;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&, scoreTextEntity, titleTextEntity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_scoreIndex = (m_scoreIndex + 1) % m_highScores.size();
                    scoreTextEntity.getComponent<xy::Text>().setString(m_highScores[m_scoreIndex]);
                    titleTextEntity.getComponent<xy::Text>().setString(m_scoreTitles[m_scoreIndex]);
                }
            });

    parentTx.addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(bounds.width - (18.f + buttonSize.width), 158.f);
    entity.addComponent<xy::UIHitBox>().area = buttonSize;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&, scoreTextEntity, titleTextEntity](xy::Entity, sf::Uint64 flags) mutable
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    m_scoreIndex = m_scoreIndex == 0 ? m_highScores.size() - 1 : m_scoreIndex - 1;
                    scoreTextEntity.getComponent<xy::Text>().setString(m_highScores[m_scoreIndex]);
                    titleTextEntity.getComponent<xy::Text>().setString(m_scoreTitles[m_scoreIndex]);
                }
            });

    parentTx.addChild(entity.getComponent<xy::Transform>());

    buttonSize.width = 46.f;
    buttonSize.height = 26.f;
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(56.f, 151.f);
    entity.addComponent<xy::UIHitBox>().area = buttonSize;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Menu::ScoreMenu;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::ScoreHiddenPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::RootNode;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::PlanetShownPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    cmd.targetFlags = CommandID::Menu::Starfield;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<Slider>().target.y = Menu::StarfieldDownPosition;
                        e.getComponent<Slider>().active = true;
                    };
                    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                }
            });

    parentTx.addChild(entity.getComponent<xy::Transform>());
}

void MenuState::saveSettings()
{
    m_configSettings.findProperty("up")->setValue(m_sharedData.keymap.up);
    m_configSettings.findProperty("down")->setValue(m_sharedData.keymap.down);
    m_configSettings.findProperty("left")->setValue(m_sharedData.keymap.left);
    m_configSettings.findProperty("right")->setValue(m_sharedData.keymap.right);
    m_configSettings.findProperty("fire")->setValue(m_sharedData.keymap.fire);
    m_configSettings.findProperty("pickup")->setValue(m_sharedData.keymap.pickup);
    m_configSettings.findProperty("joy_fire")->setValue(static_cast<std::int32_t>(m_sharedData.keymap.joyFire));
    m_configSettings.findProperty("joy_pickup")->setValue(static_cast<std::int32_t>(m_sharedData.keymap.joyPickup));
    m_configSettings.findProperty("difficulty")->setValue(m_sharedData.difficulty);
    m_configSettings.save(xy::FileSystem::getConfigDirectory(Menu::AppName) + Menu::ConfigName);
}

void MenuState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}