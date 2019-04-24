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

#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>

#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Window/Event.hpp>

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

        float rand(vec2 pos)
        {
            float ret = dot(pos.xy, vec2(12.9898, 78.233));
            ret = mod(ret, 3.14);
            return fract(sin(ret + u_time) * 43758.5453);
        }

        void main()
        {
            gl_FragColor = vec4(vec3(rand(floor((gl_FragCoord.xy /*/ vec2(720.0, 960.0)*/) / 4.0))), 1.0);
        })";
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_gameScene (ctx.appInstance.getMessageBus()),
    m_mapLoader (m_sprites)
{
    launchLoadingScreen();

    sd.shaders = &m_shaders;

    initScene();
    loadAssets();
    loadWorld();

    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    quitLoadingScreen();
}

GameState::~GameState()
{
    m_sharedData.shaders = nullptr;
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
            break;
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed
        && evt.joystickButton.joystickId == 0)
    {
        if (evt.joystickButton.button == 7)
        {
            m_sharedData.pauseMessage = SharedData::Paused;
            requestStackPush(StateID::Pause);
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
        if (data.type == BombEvent::Exploded)
        {
            m_mapLoader.renderSprite(xy::Util::Random::value(SpriteID::Crater01, SpriteID::Crater04), data.position);
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
                e.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Noise));
                e.getComponent<xy::Drawable>().setDepth(-ConstVal::BackgroundDepth);

                showCrashMessage(data.lives == 0); //has to be done here to ensure shader updated
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        else if (data.type == DroneEvent::Spawned)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::BackgroundTop;
            cmd.action = [&](xy::Entity e, float)
            {
                e.getComponent<xy::Drawable>().setShader(nullptr);
                e.getComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
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
                    drone.battery = 100.f;

                    e.getComponent<xy::Sprite>().setColour(sf::Color::White);

                    auto* msg = getContext().appInstance.getMessageBus().post<DroneEvent>(MessageID::DroneMessage);
                    msg->type = DroneEvent::Spawned;
                    msg->lives = drone.lives;
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
    m_gameScene.addSystem<DroneSystem>(mb);
    m_gameScene.addSystem<BombSystem>(mb);
    m_gameScene.addSystem<ItemBarSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::TextSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
    m_gameScene.addSystem<xy::AudioSystem>(mb);

    m_gameScene.addDirector<PlayerDirector>();
    m_gameScene.addDirector<SpawnDirector>(m_sprites);
    m_gameScene.addDirector<SFXDirector>(m_resources);

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
    recalcViews();

    m_gameScene.setActiveListener(m_topCamera);
}

void GameState::loadAssets()
{
    m_shaders.preload(ShaderID::Cloud, cloudFrag, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Noise, noiseFrag, sf::Shader::Fragment);

    TextureID::handles[TextureID::Sidebar] = m_resources.load<sf::Texture>("assets/images/sidebar.png");
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

    m_mapLoader.load("assets/maps/01.tmx");
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
    entity.addComponent<xy::Transform>().setPosition(13.f, 28.f);
    entity.addComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::AmmoIcon].getTexture());
    entity.addComponent<ItemBar>().xCount = 5;
    entity.getComponent<ItemBar>().itemCount = Drone::StartAmmo;
    entity.getComponent<ItemBar>().textureRect = m_sprites[SpriteID::AmmoIcon].getTextureRect();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::AmmoMeter;
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
    entity.addComponent<xy::Transform>().setPosition(9.f, 28.f);
    entity.addComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::Drone].getTexture());
    entity.addComponent<ItemBar>().xCount = 4;
    entity.getComponent<ItemBar>().itemCount = Drone::StartLives;
    entity.getComponent<ItemBar>().textureRect = m_sprites[SpriteID::Drone].getTextureRect();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::LifeMeter;
    otherSideTx.addChild(entity.getComponent<xy::Transform>());

    m_sideCamera.getComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition + (xy::DefaultSceneSize / 2.f));

    //then put top view relative to 0,0 as we'll be flying around over it
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_mapLoader.getTopDownTexture());
    entity.addComponent<xy::CommandTarget>().ID = CommandID::BackgroundTop;

    //create a crosshair for the drone and have the camera follow it
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::MapArea.width / 2.f, ConstVal::SmallViewSize.y / 2.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Crosshair];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::PlayerTop;
    entity.addComponent<Drone>().camera = m_topCamera;
    entity.getComponent<xy::Transform>().addChild(m_topCamera.getComponent<xy::Transform>());
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    m_topCamera.getComponent<xy::Transform>().setPosition(bounds.width / 2.f, bounds.height / 2.f);

    //this is our side-view drone
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x, ConstVal::DroneHeight);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.getComponent<xy::Transform>().setOrigin(4.f, 4.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Drone];
    entity.addComponent<xy::CommandTarget>().ID = CommandID::PlayerSide;
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
}

void GameState::showCrashMessage(bool gameOver)
{
    m_sharedData.pauseMessage = gameOver ? SharedData::GameOver : SharedData::Died;
    requestStackPush(StateID::Pause);
}