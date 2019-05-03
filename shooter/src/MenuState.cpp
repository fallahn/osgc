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
#include "StateIDs.hpp"
#include "ResourceIDs.hpp"
#include "PluginExport.hpp"
#include "SliderSystem.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>

#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/core/FileSystem.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
    const std::string TextFrag =
        R"(
    #version 120

    uniform sampler2D u_texture;

    void main()
    {
        vec2 coord = gl_TexCoord[0].xy;
        coord.x *= (1.0 + coord.y);
        coord.x -= (0.5 * coord.y);

        gl_FragColor = texture2D(u_texture, coord) * gl_Color;
    })";

    const std::int32_t HelpDepth = 10;

    const std::string AppName("drone_drop");
    const std::string ConfigName("settings.cfg");
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_scene         (ctx.appInstance.getMessageBus()),
    m_textCrawl     (sd.resources.get<sf::Font>(FontID::handles[FontID::CGA])),
    m_menuActive    (false)
{
    launchLoadingScreen();

    initScene();
    loadAssets();
    buildMenu();
    buildStarfield();
    buildHelp();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    registerConsoleTab("Options", [&]()
        {
            static const std::array<std::int32_t, 3u> Difficulty = { 3,2,1 };
            static std::int32_t index = 2; //TODO remember this across sessions
            xy::Nim::simpleCombo("Difficulty", index, "Easy\0Medium\0Hard\0\0");

            m_sharedData.difficulty = Difficulty[index];
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
    if (!m_configSettings.loadFromFile(xy::FileSystem::getConfigDirectory(AppName) + ConfigName))
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
        m_configSettings.save(xy::FileSystem::getConfigDirectory(AppName) + ConfigName);
    }

    //TODO validate loaded settings incase someone dicked around with the file
    LOG("MUST validate loaded settings file...", xy::Logger::Type::Warning);

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
    m_scene.addSystem<SliderSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
    m_scene.addSystem<xy::AudioSystem>(mb);
}

void MenuState::loadAssets()
{
    m_crawlShader.loadFromMemory(TextFrag, sf::Shader::Fragment);

    TextureID::handles[TextureID::MenuBackground] = m_resources.load<sf::Texture>("assets/images/menu_background.png");
    TextureID::handles[TextureID::HowToPlay] = m_resources.load<sf::Texture>("assets/images/how_to_play.png");

}

void MenuState::buildMenu()
{
    //root node for sliding
    auto rootNode = m_scene.createEntity();
    rootNode.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y);
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
    entity.addComponent<xy::Drawable>();
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
    entity.addComponent<xy::Drawable>();

    auto textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::LeftMouse)
        {
            requestStackClear();
            requestStackPush(StateID::Game);
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
    entity.addComponent<xy::Drawable>();

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
    entity.getComponent<xy::Text>().setString("How To Play");
    entity.getComponent<xy::Text>().setCharacterSize(Menu::ItemCharSize);
    entity.getComponent<xy::Text>().setFillColour(Menu::TextColour);
    entity.addComponent<xy::Drawable>();

    textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    LOG("Show the help screen", xy::Logger::Type::Info);
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
    entity.addComponent<xy::Drawable>();

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
    entity.addComponent<xy::Drawable>();

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
    entity.addComponent<xy::Drawable>();
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, -xy::DefaultSceneSize.y);
    entity.addComponent<xy::Drawable>();
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
    rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());


    //planet sprite
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(-10);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::MenuBackground]));
    entity.addComponent<xy::AudioEmitter>().setSource("assets/sound/menu.ogg");
    entity.getComponent<xy::AudioEmitter>().setChannel(0);
    entity.getComponent<xy::AudioEmitter>().setAttenuation(0.f);
    entity.getComponent<xy::AudioEmitter>().setLooped(true);
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
    entity.addComponent<xy::Drawable>().setDepth(-20);
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
    entity.addComponent<xy::Drawable>().setDepth(-20);
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
    entity.addComponent<xy::Drawable>().setDepth(-20);
    auto& verts3 = entity.getComponent<xy::Drawable>().getVertices();
    verts3.clear();

    for (auto p : positions)
    {
        addQuad(verts3, p);
    }
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.addComponent<Slider>().speed = 1.f;
    entity.getComponent<Slider>().target = { -40.f, -xy::DefaultSceneSize.y / 2.f };
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Menu::Starfield;
}

void MenuState::buildHelp()
{
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(20.f, 20.f);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::HowToPlay]));
    entity.addComponent<Slider>();

    auto& parentTx = entity.getComponent<xy::Transform>();

    sf::Vector2f textPos(324.f, 10.f);
    const sf::Vector2f textScale(0.25f, 0.25f);
    const float verticalSpacing = 26.f;
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]);
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth + 1);
    entity.addComponent<xy::Text>(font).setString("Controls");
    parentTx.addChild(entity.getComponent<xy::Transform>());

    textPos.y += verticalSpacing * 2.f;
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth + 1);
    entity.addComponent<xy::Text>(font).setString("Up");
    parentTx.addChild(entity.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth + 1);
    entity.addComponent<xy::Text>(font).setString("Down");
    parentTx.addChild(entity.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth + 1);
    entity.addComponent<xy::Text>(font).setString("Left");
    parentTx.addChild(entity.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth + 1);
    entity.addComponent<xy::Text>(font).setString("Right");
    parentTx.addChild(entity.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth + 1);
    entity.addComponent<xy::Text>(font).setString("Drop\nBomb");
    parentTx.addChild(entity.getComponent<xy::Transform>());

    textPos.y += verticalSpacing;
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(textPos);
    entity.getComponent<xy::Transform>().setScale(textScale);
    entity.addComponent<xy::Drawable>().setDepth(HelpDepth + 1);
    entity.addComponent<xy::Text>(font).setString("Collect\nItem");
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
    m_configSettings.findProperty("joy_fire")->setValue(m_sharedData.keymap.joyFire);
    m_configSettings.findProperty("joy_pickup")->setValue(m_sharedData.keymap.joyPickup);
    m_configSettings.findProperty("difficulty")->setValue(m_sharedData.difficulty);
    m_configSettings.save(xy::FileSystem::getConfigDirectory(AppName) + ConfigName);
}

void MenuState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}