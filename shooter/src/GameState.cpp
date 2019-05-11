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

#include "GameState.hpp"
#include "StateIDs.hpp"
#include "GameConsts.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "PlayerDirector.hpp"
#include "SpawnDirector.hpp"
#include "Drone.hpp"
#include "Bomb.hpp"
#include "SoundEffectsDirector.hpp"
#include "ItemBar.hpp"
#include "CollisionTypes.hpp"
#include "ScoreDirector.hpp"
#include "BobAnimator.hpp"
#include "Navigation.hpp"
#include "Alien.hpp"
#include "Human.hpp"
#include "RadarItemSystem.hpp"

#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/resources/ResourceHandler.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

namespace
{
    const std::string cloudFrag = R"(
        #version 120

        uniform sampler2D u_texture;
        uniform float u_time;

        void main()
        {
            vec2 coord = gl_TexCoord[0].xy;
            coord.x += u_time;

            gl_FragColor = texture2D(u_texture, coord);

        })";

    const std::string noiseFrag = R"(
        #version 120

        uniform float u_time;
        uniform vec2 u_windowSize;
        uniform vec2 u_viewSize;
        uniform sampler2D u_texture;
        uniform float u_mix;

        float rand(vec2 pos)
        {
            return fract(sin(dot(pos, vec2(12.9898, 4.1414) + u_time)) * 43758.5453);
        }

        void main()
        {
            vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy);
            vec4 noise = vec4(vec3(rand(floor((gl_FragCoord.xy / u_windowSize) * (u_viewSize / 4.0)))), 1.0);

            gl_FragColor = mix(colour, noise, u_mix);

        })";

    const std::string waterFrag = R"(
        #version 120

        uniform float u_time;
        uniform sampler2D u_texture;

        void main()
        {
            vec2 coord = gl_TexCoord[0].xy;
            float c = texture2D(u_texture, coord * 0.9 + vec2(u_time * -10.0)).r;
            c *= texture2D(u_texture, coord * 1.1 + vec2(u_time * 10.0)).g;
            c = pow(c, 5.0);

            gl_FragColor = vec4(vec3(c * 10.0), 1.0);

        })";
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_gameScene (ctx.appInstance.getMessageBus(), 1024),
    m_audioScape(m_audioResource),
    m_mapLoader (m_sprites)
{
    launchLoadingScreen();

    sd.shaders = &m_shaders;

    initScene();
    loadAssets();
    loadWorld();

    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    recalcViews();

    xy::App::setMouseCursorVisible(false);

    quitLoadingScreen();
}

GameState::~GameState()
{
    m_sharedData.shaders = nullptr;
    xy::App::setMouseCursorVisible(true);
}

//public
bool GameState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::P:
        case sf::Keyboard::Pause:
        case sf::Keyboard::Escape:
            m_sharedData.pauseMessage = SharedData::Paused;
            requestStackPush(StateID::Pause);
            m_sideCamera.getComponent<xy::AudioEmitter>().pause();
            break;
        /*case sf::Keyboard::K:
            m_sharedData.pauseMessage = SharedData::GameOver;
            m_sharedData.gameoverType = SharedData::Lose;
            requestStackPush(StateID::GameOver);
            break;
        case sf::Keyboard::L:
            m_sharedData.pauseMessage = SharedData::GameOver;
            m_sharedData.gameoverType = SharedData::Win;
            requestStackPush(StateID::GameOver);
            break;*/
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed
        && evt.joystickButton.joystickId == 0)
    {
        if (evt.joystickButton.button == 7)
        {
            m_sharedData.pauseMessage = SharedData::Paused;
            requestStackPush(StateID::Pause);
            m_sideCamera.getComponent<xy::AudioEmitter>().pause();
        }
    }

    m_gameScene.forwardEvent(evt);

    return false;
}

void GameState::handleMessage(const xy::Message& msg)
{
    if (msg.id == xy::Message::WindowMessage) 
    {
        const auto& data = msg.getData<xy::Message::WindowEvent>();
        if (data.type == xy::Message::WindowEvent::Resized)
        {
            recalcViews();
        }
    }
    else if (msg.id == MessageID::BombMessage)
    {
        const auto& data = msg.getData<BombEvent>();
        switch (data.type)
        {
        default: break;
        case BombEvent::Exploded:
            drawCrater(data.position);
            break;
        case BombEvent::KilledBeetle:
            m_mapLoader.renderSprite(SpriteID::BeetleBody, data.position, data.rotation);
            break;
        case BombEvent::KilledHuman:
            m_mapLoader.renderSprite(SpriteID::HumanBody, data.position, data.rotation);
            break;
        case BombEvent::KilledScorpion:
            m_mapLoader.renderSprite(SpriteID::ScorpionBody, data.position, data.rotation);
            break;
        }
    }
    else if (msg.id == MessageID::DroneMessage)
    {
        const auto& data = msg.getData<DroneEvent>();
        if (data.type == DroneEvent::Died)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::BackgroundTop;
            cmd.action = [&, data](xy::Entity e, float)
            {
                e.getComponent<xy::Drawable>().setDepth(-ConstVal::BackgroundDepth);

                showCrashMessage(m_sharedData.playerData.lives == 0); //has to be done here to ensure shader updated
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            m_shaders.get(ShaderID::Noise).setUniform("u_mix", 1.f);
        }
        else if (data.type == DroneEvent::Spawned)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::BackgroundTop;
            cmd.action = [&](xy::Entity e, float)
            {
                //e.getComponent<xy::Drawable>().setShader(nullptr);
                e.getComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            m_shaders.get(ShaderID::Noise).setUniform("u_mix", 0.f);
        }
        /*else if (data.type == DroneEvent::GotBattery)
        {
            m_shaders.get(ShaderID::Noise).setUniform("u_mix", 0.f);
        }
        else if (data.type == DroneEvent::BatteryLow)
        {
            m_shaders.get(ShaderID::Noise).setUniform("u_mix", 0.15f);
        }*/
        else if (data.type == DroneEvent::CollisionStart)
        {
            m_shaders.get(ShaderID::Noise).setUniform("u_mix", 0.5f);
        }
        else if (data.type == DroneEvent::CollisionEnd)
        {
            m_shaders.get(ShaderID::Noise).setUniform("u_mix", 0.f);
        }
        else if (data.type == DroneEvent::BatteryLow)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::BatteryWarningText;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<xy::Callback>().active = true;
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
    else if (msg.id == xy::Message::StateMessage)
    {
        const auto& data = msg.getData<xy::Message::StateEvent>();
        if (data.type == xy::Message::StateEvent::Popped
            && data.id == StateID::Pause)
        {
            if (m_sharedData.pauseMessage == SharedData::Died)
            {
                //respawn the player
                xy::Command cmd;
                cmd.targetFlags = CommandID::PlayerTop;
                cmd.action = [&](xy::Entity e, float)
                {
                    auto& drone = e.getComponent<Drone>();
                    drone.reset();
                    drone.battery = Drone::StartBattery;
                    drone.health = Drone::StartHealth;

                    e.getComponent<xy::Sprite>().setColour(sf::Color::White);

                    auto* msg = getContext().appInstance.getMessageBus().post<DroneEvent>(MessageID::DroneMessage);
                    msg->type = DroneEvent::Spawned;
                    msg->position = e.getComponent<xy::Transform>().getPosition();
                };
                m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                cmd.targetFlags = CommandID::PlayerSide;
                cmd.action = [](xy::Entity e, float)
                {
                    auto& tx = e.getComponent<xy::Transform>();
                    auto pos = tx.getPosition();
                    pos.y = ConstVal::DroneHeight;
                    tx.setPosition(pos);
                };
                m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
            else if (m_sharedData.pauseMessage == SharedData::Paused)
            {
                m_sideCamera.getComponent<xy::AudioEmitter>().play();
            }
            xy::App::setMouseCursorVisible(false);
        }
    }
    else if (msg.id == MessageID::HumanMessage)
    {
        const auto& data = msg.getData<HumanEvent>();
        if (data.type == HumanEvent::Died)
        {
            m_mapLoader.renderSprite(SpriteID::HumanBody, data.position, data.rotation);
        }
    }
    else if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        if (data.type == GameEvent::StateChange)
        {
            switch (data.reason)
            {
            default: break;
            case GameEvent::NoAliensLeft:
                m_sharedData.pauseMessage = SharedData::GameOver;
                m_sharedData.gameoverType = SharedData::Win;
                requestStackPush(StateID::GameOver);
                break;
            case GameEvent::NoHumansLeft:
                m_sharedData.pauseMessage = SharedData::GameOver;
                m_sharedData.gameoverType = SharedData::Lose;
                requestStackPush(StateID::GameOver);
                break;
            }
        }
    }

    m_gameScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
    static float shaderTime = 0.f;
    shaderTime += dt * 0.001f;
    m_shaders.get(ShaderID::Cloud).setUniform("u_time", shaderTime);
    m_shaders.get(ShaderID::Noise).setUniform("u_time", shaderTime);
    m_shaders.get(ShaderID::Water).setUniform("u_time", shaderTime);

    m_gameScene.update(dt);

    return false;
}

void GameState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_gameScene);

    m_gameScene.setActiveCamera(m_topCamera);
    rw.draw(m_gameScene);

    m_gameScene.setActiveCamera(m_sideCamera);
}

xy::StateID GameState::stateID() const
{
    return StateID::Game;
}

//private
void GameState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<DroneSystem>(mb, m_sprites, m_sharedData);
    m_gameScene.addSystem<AlienSystem>(mb, m_sprites, m_sharedData.difficulty);
    m_gameScene.addSystem<HumanSystem>(mb, m_sprites, m_sharedData.difficulty);
    m_gameScene.addSystem<BombSystem>(mb);
    m_gameScene.addSystem<ItemBarSystem>(mb);
    m_gameScene.addSystem<BobSystem>(mb);
    m_gameScene.addSystem<RadarItemSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::TextSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
    m_gameScene.addSystem<xy::AudioSystem>(mb);

    m_gameScene.addDirector<PlayerDirector>(m_sharedData.keymap);
    m_gameScene.addDirector<SpawnDirector>(m_sprites);
    m_gameScene.addDirector<SFXDirector>(m_resources);
    m_gameScene.addDirector<ScoreDirector>(m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA]), m_sharedData.difficulty, m_sharedData);

    m_sideCamera = m_gameScene.createEntity();
    m_sideCamera.addComponent<xy::Transform>();
    m_sideCamera.addComponent<xy::Camera>();
    m_gameScene.setActiveCamera(m_sideCamera);
    //view properties will be set in ctor now this is active

    m_topCamera = m_gameScene.createEntity();
    m_topCamera.addComponent<xy::Transform>();
    m_topCamera.addComponent<xy::Camera>().setView(ConstVal::SmallViewSize);
    m_topCamera.getComponent<xy::Camera>().setBounds(ConstVal::MapArea);
    m_topCamera.addComponent<xy::AudioListener>();

    m_gameScene.setActiveListener(m_topCamera);
}

void GameState::loadAssets()
{
    m_shaders.preload(ShaderID::Cloud, cloudFrag, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Water, waterFrag, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Noise, noiseFrag, sf::Shader::Fragment);
    m_shaders.get(ShaderID::Noise).setUniform("u_windowSize", sf::Vector2f(getContext().renderWindow.getSize()));
    m_shaders.get(ShaderID::Noise).setUniform("u_viewSize", ConstVal::SmallViewSize);

    TextureID::handles[TextureID::Sidebar] = m_resources.load<sf::Texture>("assets/images/sidebar.png");
    TextureID::handles[TextureID::Noise] = m_resources.load<sf::Texture>("assets/images/noise.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::Noise]).setRepeated(true);
    TextureID::handles[TextureID::Clouds] = m_resources.load<sf::Texture>("assets/images/clouds.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::Clouds]).setRepeated(true);

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/explosion.spt", m_resources);
    m_sprites[SpriteID::Explosion] = spriteSheet.getSprite("explosion");

    spriteSheet.loadFromFile("assets/sprites/radar.spt", m_resources);
    m_sprites[SpriteID::BuildingLeft] = spriteSheet.getSprite("building_left");
    m_sprites[SpriteID::BuildingCentre] = spriteSheet.getSprite("building_centre");
    m_sprites[SpriteID::BuildingRight] = spriteSheet.getSprite("building_right");
    m_sprites[SpriteID::HillLeftWide] = spriteSheet.getSprite("hill_left_wide");
    m_sprites[SpriteID::HillLeftNarrow] = spriteSheet.getSprite("hill_left_narrow");
    m_sprites[SpriteID::HillCentre] = spriteSheet.getSprite("hill_centre");
    m_sprites[SpriteID::HillRightNarrow] = spriteSheet.getSprite("hill_right_narrow");
    m_sprites[SpriteID::HillRightWide] = spriteSheet.getSprite("hill_right_wide");
    m_sprites[SpriteID::TankIcon] = spriteSheet.getSprite("tank");
    m_sprites[SpriteID::TreeIcon] = spriteSheet.getSprite("tree");
    m_sprites[SpriteID::BushIcon] = spriteSheet.getSprite("bush");
    m_sprites[SpriteID::Drone] = spriteSheet.getSprite("drone");
    m_sprites[SpriteID::ScorpionIcon] = spriteSheet.getSprite("scorpion");
    m_sprites[SpriteID::AmmoIcon] = spriteSheet.getSprite("ammo");
    m_sprites[SpriteID::BatteryIcon] = spriteSheet.getSprite("battery");
    m_sprites[SpriteID::BeetleIcon] = spriteSheet.getSprite("beetle");

    spriteSheet.loadFromFile("assets/sprites/ui.spt", m_resources);
    m_sprites[SpriteID::AmmoTop] = spriteSheet.getSprite("ammo");
    m_sprites[SpriteID::Crosshair] = spriteSheet.getSprite("crosshair");
    m_sprites[SpriteID::BatteryTop] = spriteSheet.getSprite("battery");
    m_sprites[SpriteID::HealthBar] = spriteSheet.getSprite("health_bar");
    m_sprites[SpriteID::BatteryBar] = spriteSheet.getSprite("battery_bar");

    spriteSheet.loadFromFile("assets/sprites/craters.spt", m_resources);
    m_sprites[SpriteID::Crater01] = spriteSheet.getSprite("crater_01");
    m_sprites[SpriteID::Crater02] = spriteSheet.getSprite("crater_02");
    m_sprites[SpriteID::Crater03] = spriteSheet.getSprite("crater_03");
    m_sprites[SpriteID::Crater04] = spriteSheet.getSprite("crater_04");

    spriteSheet.loadFromFile("assets/sprites/mini_explosion.spt", m_resources);
    m_sprites[SpriteID::ExplosionIcon] = spriteSheet.getSprite("explosion");

    spriteSheet.loadFromFile("assets/sprites/human.spt", m_resources);
    m_sprites[SpriteID::Human] = spriteSheet.getSprite("human");

    spriteSheet.loadFromFile("assets/sprites/scorpion.spt", m_resources);
    m_sprites[SpriteID::Scorpion] = spriteSheet.getSprite("scorpion");

    spriteSheet.loadFromFile("assets/sprites/beetle.spt", m_resources);
    m_sprites[SpriteID::Beetle] = spriteSheet.getSprite("beetle");

    spriteSheet.loadFromFile("assets/sprites/bodies.spt", m_resources);
    m_sprites[SpriteID::HumanBody] = spriteSheet.getSprite("human");
    m_sprites[SpriteID::BeetleBody] = spriteSheet.getSprite("beetle");
    m_sprites[SpriteID::ScorpionBody] = spriteSheet.getSprite("scorpion");

    spriteSheet.loadFromFile("assets/sprites/sidebar.spt", m_resources);
    m_sprites[SpriteID::Science01] = spriteSheet.getSprite("science");
    m_sprites[SpriteID::Science02] = spriteSheet.getSprite("more_science");

    m_audioScape.loadFromFile("assets/sound/game.xas");

    std::string mapfile = "assets/maps/" + m_sharedData.mapNames[m_sharedData.playerData.currentMap];
    if(!m_mapLoader.load(mapfile))
    {
        m_sharedData.messageString = "Failed To Load Map";
        m_sharedData.pauseMessage = SharedData::Error;
        requestStackPush(StateID::Pause);
        xy::Logger::log("Couldn't open " + mapfile, xy::Logger::Type::Error);
    }
}

void GameState::loadWorld()
{
    //the background is static so let's put that off to one side along with the camera
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_mapLoader.getSideTexture());

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Cloud));
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", m_resources.get<sf::Texture>(TextureID::handles[TextureID::Clouds]));
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Clouds]));

    //left side bar
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition);
    entity.getComponent<xy::Transform>().move(0.f, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Sidebar]));
    float sidebarTextureWidth = entity.getComponent<xy::Sprite>().getTextureBounds().width;

    auto& sideTx = entity.getComponent<xy::Transform>();
    //battery meter
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(5.f, 8.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::BatteryBar];
    entity.getComponent<xy::Sprite>().setColour(sf::Color::Green);
    entity.addComponent<sf::FloatRect>() = m_sprites[SpriteID::BatteryBar].getTextureRect();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::BatteryMeter;
    sideTx.addChild(entity.getComponent<xy::Transform>());

    //ammo meter
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(11.f, 28.f);
    entity.addComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::AmmoIcon].getTexture());
    entity.addComponent<ItemBar>().xCount = 5;
    entity.getComponent<ItemBar>().itemCount = Drone::StartAmmo;
    entity.getComponent<ItemBar>().textureRect = m_sprites[SpriteID::AmmoIcon].getTextureRect();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::AmmoMeter;
    sideTx.addChild(entity.getComponent<xy::Transform>());

    //score
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x + (xy::DefaultSceneSize.x / 2.f), 10.f);// (11.f, 56.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA])).setString("Score: " + std::to_string(m_sharedData.playerData.score));
    entity.getComponent<xy::Text>().setCharacterSize(48);
    entity.getComponent<xy::Text>().setFillColour({ 0,0,128 });
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ScoreText;

    //battery warning
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x + (xy::DefaultSceneSize.x / 2.f), 80.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA])).setString("BATTERY LOW");
    entity.getComponent<xy::Text>().setCharacterSize(160);
    entity.getComponent<xy::Text>().setFillColour({ 255,0,0,0 });
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::BatteryWarningText;
    entity.addComponent<xy::Callback>().userData = std::make_any<std::pair<float, float>>(std::make_pair(0.5f, 6.f));
    entity.getComponent<xy::Callback>().function =
        [](xy::Entity e, float dt) 
    {
        auto& [flashTime, duration] = std::any_cast<std::pair<float, float>&>(e.getComponent<xy::Callback>().userData);
        flashTime -= dt;
        duration -= dt;

        if (flashTime < 0.f)
        {
            flashTime = 0.5f;

            auto colour = e.getComponent<xy::Text>().getFillColour();
            colour.a = (colour.a == 0) ? 255 : 0;
            e.getComponent<xy::Text>().setFillColour(colour);
        }

        if (duration < 0.f)
        {
            flashTime = 0.5f;
            duration = 6.f;

            auto colour = e.getComponent<xy::Text>().getFillColour();
            colour.a = 0;
            e.getComponent<xy::Text>().setFillColour(colour);
            e.getComponent<xy::Callback>().active = false;
        }
    };

    //human count
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(sidebarTextureWidth / 2.f, 140.f);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA])).setString("Humans");
    entity.getComponent<xy::Text>().setCharacterSize(24);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    sideTx.addChild(entity.getComponent<xy::Transform>());

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(sidebarTextureWidth / 2.f, 167.f);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA])).setString(std::to_string(Human::NumberPerRound / m_sharedData.difficulty));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::HumanCount;
    sideTx.addChild(entity.getComponent<xy::Transform>());

    //wobbly widget
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(8.f, 60.f);
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Science01];
    entity.addComponent<xy::SpriteAnimation>().play(0);
    sideTx.addChild(entity.getComponent<xy::Transform>());

    //right side bar
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition);
    entity.getComponent<xy::Transform>().move(0.f, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Sidebar]));
    float sidebarWidth = entity.getComponent<xy::Sprite>().getTextureBounds().width * 4.f;
    sidebarWidth = xy::DefaultSceneSize.x - sidebarWidth;
    entity.getComponent<xy::Transform>().move(sidebarWidth, 0.f);

    //health meter
    auto& otherSideTx = entity.getComponent<xy::Transform>();
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(5.f, 8.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::HealthBar];
    entity.getComponent<xy::Sprite>().setColour(sf::Color::Green);
    entity.addComponent<sf::FloatRect>() = m_sprites[SpriteID::HealthBar].getTextureRect();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::HealthMeter;
    otherSideTx.addChild(entity.getComponent<xy::Transform>());

    //life bar
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(8.f, 28.f);
    entity.addComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::Drone].getTexture());
    entity.addComponent<ItemBar>().xCount = 4;
    entity.getComponent<ItemBar>().itemCount = m_sharedData.playerData.lives;
    entity.getComponent<ItemBar>().textureRect = m_sprites[SpriteID::Drone].getTextureRect();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::LifeMeter;
    otherSideTx.addChild(entity.getComponent<xy::Transform>());

    //alien count
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(sidebarTextureWidth / 2.f, 140.f);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA])).setString("Aliens");
    entity.getComponent<xy::Text>().setCharacterSize(24);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    otherSideTx.addChild(entity.getComponent<xy::Transform>());

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(sidebarTextureWidth / 2.f, 167.f);
    entity.getComponent<xy::Transform>().setScale(0.25f, 0.25f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::CGA])).setString(std::to_string(Alien::NumberPerRound / m_sharedData.difficulty));
    entity.getComponent<xy::Text>().setCharacterSize(32);
    entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::AlienCount;
    otherSideTx.addChild(entity.getComponent<xy::Transform>());

    //wobbly widget
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(8.f, 60.f);
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Science02];
    entity.addComponent<xy::SpriteAnimation>().play(0);
    otherSideTx.addChild(entity.getComponent<xy::Transform>());


    m_sideCamera.getComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition + (xy::DefaultSceneSize / 2.f));
    m_sideCamera.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("music");
    m_sideCamera.getComponent<xy::AudioEmitter>().play();

    //then put top view relative to 0,0 as we'll be flying around over it
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Noise));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<xy::Sprite>(m_mapLoader.getTopDownTexture());
    entity.addComponent<xy::CommandTarget>().ID = CommandID::BackgroundTop;

    //create a crosshair for the drone and have the camera follow it
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::MapArea.width / 2.f, ConstVal::MapArea.height / 2.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Crosshair];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::PlayerTop;
    entity.addComponent<Drone>().camera = m_topCamera;
    entity.getComponent<xy::Transform>().addChild(m_topCamera.getComponent<xy::Transform>());
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("empty");
    entity.getComponent<xy::AudioEmitter>().setChannel(1);
    m_topCamera.getComponent<xy::Transform>().setPosition(bounds.width / 2.f, bounds.height / 2.f);

    //this is our side-view drone
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x, ConstVal::DroneHeight);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.getComponent<xy::Transform>().setOrigin(4.f, 4.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Drone];
    entity.addComponent<xy::CommandTarget>().ID = CommandID::PlayerSide;

    //load up any collision objects
    const auto& boxes = m_mapLoader.getCollisionBoxes();
    for (const auto& b : boxes)
    {
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(b.worldBounds.left, b.worldBounds.top);
        entity.addComponent<CollisionBox>() = b;
        //scale the objects so anything with a texture has correct pixel size
        sf::Vector2f size(b.worldBounds.width / 4.f, b.worldBounds.height / 4.f);
        entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
        entity.addComponent<xy::BroadphaseComponent>().setArea({ 0.f, 0.f, size.x, size.y });
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(b.filter);

        if (b.type == CollisionBox::Water)
        {
            //add a sparkly water sprite
            entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth + 1);
            auto& verts = entity.getComponent<xy::Drawable>().getVertices();
            verts.resize(4);
            verts[0] = { sf::Vector2f(), sf::Color::Magenta };
            verts[1] = { sf::Vector2f(size.x, 0.f), sf::Color::Magenta, sf::Vector2f(size.x, 0.f) };
            verts[2] = { size, sf::Color::Magenta, size };
            verts[3] = { sf::Vector2f(0.f, size.y), sf::Color::Magenta, sf::Vector2f(0.f, size.y) };

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Water));
            entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
            entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(TextureID::handles[TextureID::Noise]));
            entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
        }
    }

    /*auto debugEnt = m_gameScene.createEntity();
    debugEnt.addComponent<xy::Transform>();
    debugEnt.addComponent<xy::Drawable>().setPrimitiveType(sf::LineStrip);
    auto& verts = debugEnt.getComponent<xy::Drawable>().getVertices();*/

    //and navigation nodes
    std::uint32_t nodeID = 1;
    const auto& nodes = m_mapLoader.getNavigationNodes();
    for (const auto& n : nodes)
    {
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(n.front());
        entity.addComponent<Node>().ID = nodeID; //if we give the return path (below) the same ID we won't walk back along the same one
        entity.getComponent<Node>().path = n;
        entity.addComponent<xy::BroadphaseComponent>().setArea({ Node::Bounds[0], Node::Bounds[1], Node::Bounds[2], Node::Bounds[3] });
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionBox::Navigation);

        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(n.back());
        entity.addComponent<Node>().ID = nodeID;
        entity.getComponent<Node>().path = n;
        std::reverse(entity.getComponent<Node>().path.begin(), entity.getComponent<Node>().path.end());
        entity.addComponent<xy::BroadphaseComponent>().setArea({ Node::Bounds[0], Node::Bounds[1], Node::Bounds[2], Node::Bounds[3] });
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionBox::Navigation);

        nodeID++;

        /*verts.emplace_back(n.front(), sf::Color::Transparent);
        for (auto p : n) verts.emplace_back(p, sf::Color::Red);
        verts.emplace_back(n.back(), sf::Color::Transparent);*/
    }
    //debugEnt.getComponent<xy::Drawable>().updateLocalBounds();

    //finally spawn points
    auto& alienSystem = m_gameScene.getSystem<AlienSystem>();
    auto& humanSystem = m_gameScene.getSystem<HumanSystem>();

    alienSystem.clearSpawns();
    humanSystem.clearSpawns();

    const auto& spawns = m_mapLoader.getSpawnPoints();
    for (const auto& [position, type] : spawns)
    {
        if (type == MapLoader::Alien)
        {
            alienSystem.addSpawn(position);
        }
        else if (type == MapLoader::Human)
        {
            humanSystem.addSpawn(position);
        }
    }
    
    //saves a bit of memory
    m_mapLoader.clearObjectData();
}

void GameState::recalcViews()
{
    sf::FloatRect newView;
    sf::FloatRect largeView = getContext().defaultView.getViewport();

    m_sideCamera.getComponent<xy::Camera>().setViewport(largeView);

    newView.left = largeView.left + (ConstVal::SmallViewPort.left * largeView.width);
    newView.top = largeView.top + (ConstVal::SmallViewPort.top * largeView.height);
    newView.width = largeView.width * ConstVal::SmallViewPort.width;
    newView.height = largeView.height * ConstVal::SmallViewPort.height;

    m_topCamera.getComponent<xy::Camera>().setViewport(newView);

    m_shaders.get(ShaderID::Noise).setUniform("u_windowSize", sf::Vector2f(getContext().renderWindow.getSize()));
}

void GameState::showCrashMessage(bool gameOver)
{
    if (gameOver)
    {
        m_sharedData.pauseMessage = SharedData::GameOver;
        m_sharedData.gameoverType = SharedData::Lose;
        requestStackPush(StateID::GameOver);
    }
    else
    {
        m_sharedData.pauseMessage = SharedData::Died;
        requestStackPush(StateID::Pause);
    }
}

void GameState::drawCrater(sf::Vector2f position)
{
    //check we're not in water first
    sf::FloatRect query(position.x - 16.f, position.y - 16.f, 32.f, 32.f);
    auto nearby = m_gameScene.getSystem<xy::DynamicTreeSystem>().query(query, CollisionBox::NoDecal);
    for (auto e : nearby)
    {
        auto bounds = e.getComponent<xy::Transform>().getTransform().transformRect(e.getComponent<xy::BroadphaseComponent>().getArea());
        if (bounds.contains(position))
        {
            return;
        }
    }

    m_mapLoader.renderSprite(xy::Util::Random::value(SpriteID::Crater01, SpriteID::Crater04), position);
}

void GameState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}