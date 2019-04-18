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
#include "ResourceIDs.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "PlayerDirector.hpp"
#include "Drone.hpp"
#include "Bomb.hpp"

#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>

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
    else if (msg.id == MessageID::BombMessage)
    {
        const auto& data = msg.getData<BombEvent>();
        if (data.type == BombEvent::Exploded)
        {
            auto entity = m_gameScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(data.position);
            entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
            entity.addComponent<xy::Drawable>();
            entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::Explosion];
            entity.addComponent<xy::SpriteAnimation>().play(0);

            auto bounds = SpriteID::sprites[SpriteID::Explosion].getTextureBounds();
            entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

            entity.addComponent<xy::Callback>().active = true;
            entity.getComponent<xy::Callback>().function =
                [&](xy::Entity e, float)
            {
                if (e.getComponent<xy::SpriteAnimation>().stopped())
                {
                    m_gameScene.destroyEntity(e);
                }
            };
        }
    }
    m_gameScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
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

    m_gameScene.addDirector<PlayerDirector>();

    m_sideCamera = m_gameScene.createEntity();
    m_sideCamera.addComponent<xy::Transform>();
    m_sideCamera.addComponent<xy::Camera>();
    m_gameScene.setActiveCamera(m_sideCamera);
    //view properties will be set in ctor now this is active

    m_topCamera = m_gameScene.createEntity();
    m_topCamera.addComponent<xy::Transform>();
    m_topCamera.addComponent<xy::Camera>().setView(ConstVal::SmallViewSize);
    m_topCamera.getComponent<xy::Camera>().setBounds(ConstVal::MapArea);
    recalcViews();
}

void GameState::loadAssets()
{
    m_mapLoader.load("assets/maps/01.tmx");

    //TextureID::handles[TextureID::TopView] = m_resources.load<sf::Texture>("assets/images/test_view.png");
    TextureID::handles[TextureID::CrossHair] = m_resources.load<sf::Texture>("assets/images/crosshair.png");

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/explosion.spt", m_resources);

    SpriteID::sprites[SpriteID::Explosion] = spriteSheet.getSprite("explosion");
}

void GameState::loadWorld()
{
    //the background is static so let's put that off to one side along with the camera
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition);
    entity.addComponent<xy::Drawable>().setDepth(ConstVal::BackgroundDepth);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.resize(4);
    verts[0].color = sf::Color::Magenta;
    verts[1] = { sf::Vector2f(xy::DefaultSceneSize.x, 0.f), sf::Color::Magenta };
    verts[2] = { xy::DefaultSceneSize, sf::Color::Magenta };
    verts[3] = { sf::Vector2f(0.f, xy::DefaultSceneSize.y), sf::Color::Magenta };
    entity.getComponent<xy::Drawable>().updateLocalBounds();

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
    auto bounds = entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::CrossHair])).getTextureBounds();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::PlayerTop;
    entity.addComponent<Drone>();
    entity.getComponent<xy::Transform>().addChild(m_topCamera.getComponent<xy::Transform>());
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);


    //this is our side-view drone
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(ConstVal::BackgroundPosition.x, ConstVal::DroneHeight);
    entity.addComponent<xy::Drawable>();
    auto& verts2 = entity.getComponent<xy::Drawable>().getVertices();
    verts2.resize(4);
    verts2[0] = { sf::Vector2f(-16.f, -16.f), sf::Color::Blue };
    verts2[1] = { sf::Vector2f(16.f, -16.f), sf::Color::Blue };
    verts2[2] = { sf::Vector2f(16.f, 16.f), sf::Color::Blue };
    verts2[3] = { sf::Vector2f(-16.f, 16.f), sf::Color::Blue };
    entity.getComponent<xy::Drawable>().updateLocalBounds();
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