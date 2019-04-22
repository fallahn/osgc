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
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_gameScene (ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();

    initScene();
    loadAssets();
    loadWorld();

    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    quitLoadingScreen();
}

//public
bool GameState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
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

    m_gameScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
    static float shaderTime = 0.f;
    shaderTime += dt * 0.001f;
    m_cloudShader.setUniform("u_time", shaderTime);

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
    m_cloudShader.loadFromMemory(cloudFrag, sf::Shader::Fragment);

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
    m_sprites[SpriteID::Drone] = spriteSheet.getSprite("drone");

    spriteSheet.loadFromFile("assets/sprites/ui.spt", m_resources);
    m_sprites[SpriteID::AmmoTop] = spriteSheet.getSprite("ammo");
    m_sprites[SpriteID::Crosshair] = spriteSheet.getSprite("crosshair");
    m_sprites[SpriteID::BatteryTop] = spriteSheet.getSprite("battery");

    spriteSheet.loadFromFile("assets/sprites/mini_explosion.spt", m_resources);
    m_sprites[SpriteID::ExplosionIcon] = spriteSheet.getSprite("explosion");

    m_mapLoader.load("assets/maps/01.tmx", m_sprites);
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
    entity.getComponent<xy::Drawable>().setShader(&m_cloudShader);
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", m_resources.get<sf::Texture>(TextureID::handles[TextureID::Clouds]));
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Clouds]));

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition);
    entity.getComponent<xy::Transform>().move(0.f, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Sidebar]));

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition);
    entity.getComponent<xy::Transform>().move(0.f, ConstVal::SmallViewPort.top * xy::DefaultSceneSize.y);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Sidebar]));
    float sidebarWidth = entity.getComponent<xy::Sprite>().getTextureBounds().width * 4.f;
    sidebarWidth = xy::DefaultSceneSize.x - sidebarWidth;
    entity.getComponent<xy::Transform>().move(sidebarWidth, 0.f);


    m_sideCamera.getComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition + (xy::DefaultSceneSize / 2.f));

    //then put top view relative to 0,0 as we'll be flying around over it
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setScale(4.f, 4.f);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    entity.addComponent<xy::Sprite>(m_mapLoader.getTopDownTexture());

    //create a crosshair for the drone and have the camera follow it
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::MapArea.width / 2.f, ConstVal::SmallViewSize.y / 2.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Crosshair];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::PlayerTop;
    entity.addComponent<Drone>();
    entity.getComponent<xy::Transform>().addChild(m_topCamera.getComponent<xy::Transform>());
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.getComponent<xy::Transform>().setScale(4.f, 4.f);


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