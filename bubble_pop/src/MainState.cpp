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


#include "MainState.hpp"
#include "StateIDs.hpp"
#include "GameConsts.hpp"
#include "CommandID.hpp"
#include "GameDirector.hpp"
#include "BubbleSystem.hpp"
#include "SoundEffectsDirector.hpp"
#include "MessageIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/BitmapText.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/BitmapTextSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>

#include <xyginext/graphics/BitmapFont.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/core/FileSystem.hpp>

#include <SFML/Window/Event.hpp>

namespace
{
    const std::size_t MaxLevels = 100; 
    const float TitleSpeed = 400.f;
}

MainState::MainState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_scene         (ctx.appInstance.getMessageBus(), 1024u),
    m_playerInput   (m_nodeSet)
{
    launchLoadingScreen();
    initScene();
    loadResources();

    buildArena();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    xy::App::setMouseCursorVisible(false);
    xy::App::getRenderWindow()->setMouseCursorGrabbed(true);

    quitLoadingScreen();

    //make sure to delay this by a frame
    xy::Command cmd;
    cmd.targetFlags = CommandID::ScoreString; //doesn't matter, it just has to exist
    cmd.action = [&](xy::Entity, float) 
    {
        requestStackPush(StateID::GameOver); 
    };
    m_scene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

//public
bool MainState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::P:
        case sf::Keyboard::Escape:
        case sf::Keyboard::Pause:
            requestStackPush(StateID::Pause);
            break;
        }
    }

    m_playerInput.handleEvent(evt);
    m_scene.forwardEvent(evt);

    return true;
}

void MainState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        if (data.type == GameEvent::RoundFailed
            && getStackSize() == 1)
        {
            //disabling this prevents more bubble collision events being raised
            m_scene.setSystemActive<BubbleSystem>(false);
            m_scene.setSystemActive<xy::CallbackSystem>(false);
            requestStackPush(StateID::GameOver);
        }
    }
    else if (msg.id == xy::Message::StateMessage)
    {
        const auto& data = msg.getData<xy::Message::StateEvent>();
        switch (data.type)
        {
        default: break;
        case xy::Message::StateEvent::Popped:
            if (data.id == StateID::Attract)
            {
                m_scene.setSystemActive<BubbleSystem>(true);
                m_scene.setSystemActive<xy::CallbackSystem>(true);
            }
            else if (data.id == StateID::GameOver)
            {
                m_scene.getDirector<GameDirector>().reset();
                requestStackPush(StateID::Attract);
            }
        }
    }

    m_scene.forwardMessage(msg);
}

bool MainState::update(float dt)
{
    m_playerInput.update(dt);
    m_scene.update(dt);
    return true;
}

void MainState::draw()
{
    auto& rw = getContext().renderWindow;

    rw.draw(m_scene);
}

xy::StateID MainState::stateID() const
{
    return StateID::Main;
}

//private
void MainState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<BubbleSystem>(mb, m_nodeSet);
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::BitmapTextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
    m_scene.addSystem<xy::AudioSystem>(mb);

    m_scene.addDirector<GameDirector>(m_nodeSet, m_sprites, m_animationMaps, m_sharedData).loadLevelData();
    m_scene.addDirector<SFXDirector>();

    m_scene.setSystemActive<xy::CallbackSystem>(false); //disabled until game begins
}

void MainState::loadResources()
{
    xy::SpriteSheet spriteSheet;

    spriteSheet.loadFromFile("assets/sprites/red.spt", m_resources);
    m_sprites[BubbleID::Red] = spriteSheet.getSprite("ball");
    auto* animMap = m_animationMaps[BubbleID::Red].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");

    spriteSheet.loadFromFile("assets/sprites/green.spt", m_resources);
    m_sprites[BubbleID::Green] = spriteSheet.getSprite("ball");
    animMap = m_animationMaps[BubbleID::Green].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");


    spriteSheet.loadFromFile("assets/sprites/blue.spt", m_resources);
    m_sprites[BubbleID::Blue] = spriteSheet.getSprite("ball");
    animMap = m_animationMaps[BubbleID::Blue].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");

    spriteSheet.loadFromFile("assets/sprites/cyan.spt", m_resources);
    m_sprites[BubbleID::Cyan] = spriteSheet.getSprite("ball");
    animMap = m_animationMaps[BubbleID::Cyan].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");

    spriteSheet.loadFromFile("assets/sprites/magenta.spt", m_resources);
    m_sprites[BubbleID::Magenta] = spriteSheet.getSprite("ball");
    animMap = m_animationMaps[BubbleID::Magenta].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");

    spriteSheet.loadFromFile("assets/sprites/yellow.spt", m_resources);
    m_sprites[BubbleID::Yellow] = spriteSheet.getSprite("ball");
    animMap = m_animationMaps[BubbleID::Yellow].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");

    spriteSheet.loadFromFile("assets/sprites/orange.spt", m_resources);
    m_sprites[BubbleID::Orange] = spriteSheet.getSprite("ball");
    animMap = m_animationMaps[BubbleID::Orange].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");

    spriteSheet.loadFromFile("assets/sprites/grey.spt", m_resources);
    m_sprites[BubbleID::Grey] = spriteSheet.getSprite("ball");
    animMap = m_animationMaps[BubbleID::Grey].data();
    animMap[AnimID::Bubble::Burst] = spriteSheet.getAnimationIndex("burst", "ball");
    animMap[AnimID::Bubble::Grin] = spriteSheet.getAnimationIndex("grin", "ball");
    animMap[AnimID::Bubble::Happy] = spriteSheet.getAnimationIndex("happy", "ball");
    animMap[AnimID::Bubble::Idle] = spriteSheet.getAnimationIndex("idle", "ball");
    animMap[AnimID::Bubble::Mounted] = spriteSheet.getAnimationIndex("mounted", "ball");
    animMap[AnimID::Bubble::Queued] = spriteSheet.getAnimationIndex("queued", "ball");
    animMap[AnimID::Bubble::Shoot] = spriteSheet.getAnimationIndex("shoot", "ball");
    animMap[AnimID::Bubble::Wink] = spriteSheet.getAnimationIndex("wink", "ball");


    m_textures[TextureID::Background] = m_resources.load<sf::Texture>("assets/images/background.png");
    m_textures[TextureID::RayGun] = m_resources.load<sf::Texture>("assets/images/shooter.png");
    m_textures[TextureID::GunMount] = m_resources.load<sf::Texture>("assets/images/shooter_circle.png");
}

void MainState::buildArena()
{
    sf::Vector2f playArea(m_resources.get<sf::Texture>(m_textures[TextureID::Background]).getSize());
    
    m_nodeSet.rootNode = m_scene.createEntity();
    m_nodeSet.rootNode.addComponent<xy::Transform>().setScale(2.f, 2.f);
    m_nodeSet.rootNode.getComponent<xy::Transform>().setPosition((xy::DefaultSceneSize.x - playArea.x) / 4.f, 60.f);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-10);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::Background]));
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(Const::GunPosition);
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::GunMount]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::Callback>().userData = std::make_any<bool>(false);
    entity.getComponent<xy::Callback>().function = [](xy::Entity e, float dt)
    {
        static const float Speed = 100.f;
        auto& left = std::any_cast<bool&>(e.getComponent<xy::Callback>().userData);

        auto& tx = e.getComponent<xy::Transform>();

        if (left)
        {
            tx.move(-Speed * dt, 0.f);
            auto position = tx.getPosition();
            if (position.x < Const::LeftBounds)
            {
                left = false;
                position.x = Const::LeftBounds;
                tx.setPosition(position);
            }
        }
        else
        {
            tx.move(Speed * dt, 0.f);
            auto position = tx.getPosition();
            if (position.x > Const::RightBounds)
            {
                left = true;
                position.x = Const::RightBounds;
                tx.setPosition(position);
            }
        }
    };

    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
    m_nodeSet.gunNode = entity;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_nodeSet.gunNode.getComponent<xy::Transform>().getOrigin());
    entity.getComponent<xy::Transform>().setRotation(270.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::RayGun]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(11.f, bounds.height / 2.f);
    m_nodeSet.gunNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    m_playerInput.setPlayerEntity(entity);

    m_nodeSet.barNode = m_scene.createEntity();
    m_nodeSet.barNode.addComponent<xy::Transform>().setPosition(Const::BarPosition);
    m_nodeSet.barNode.addComponent<xy::CommandTarget>().ID = CommandID::TopBar;
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(m_nodeSet.barNode.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, -m_sharedData.sprites[SpriteID::TopBar].getTextureRect().height);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sharedData.sprites[SpriteID::TopBar];
    m_nodeSet.barNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());   

    m_scene.getDirector<GameDirector>().activateLevel();

    auto& font = m_sharedData.resources.get<xy::BitmapFont>(m_sharedData.fontID);

    //score text
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(10.f, 10.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::BitmapText>(font).setString("SCORE");
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(10.f, 26.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::BitmapText>(font).setString("000000");
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ScoreString;
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //level text
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(playArea.x / 2.f, -48.f);
    entity.addComponent<xy::Drawable>().setDepth(50);
    entity.addComponent<xy::BitmapText>(font);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::TitleTextA;
    entity.addComponent<xy::Callback>();// .active = true;
    entity.getComponent<xy::Callback>().userData = std::make_any<std::int32_t>(0);
    entity.getComponent<xy::Callback>().function = 
        [playArea](xy::Entity e, float dt)
    {
        auto& state = std::any_cast<std::int32_t&>(e.getComponent<xy::Callback>().userData);
        auto& tx = e.getComponent<xy::Transform>();
        static float timer = 1.5f;
        switch(state)
        {
        case 0:
            //move down
            tx.move(0.f, TitleSpeed * dt);
            if (tx.getPosition().y > (playArea.y / 2.f) - 16.f)
            {
                state = 1;
            }
            break;
        case 1:
            //hold
            timer -= dt;
            if (timer < 0)
            {
                timer = 1.5f;
                state = 2;
            }
            break;
        case 2:
            //exit
            tx.move(TitleSpeed * dt, 0.f);
            if (tx.getPosition().x > 960)
            {
                state = 0;
                tx.setPosition(playArea.x / 2.f, -48.f);
                e.getComponent<xy::Callback>().active = false;
            }
            break;
        }
    };
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(playArea.x / 2.f, (xy::DefaultSceneSize.y / 2.f) + 1.f);
    entity.addComponent<xy::Drawable>().setDepth(50);
    entity.addComponent<xy::BitmapText>(font);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::TitleTextB;
    entity.addComponent<xy::Callback>();// .active = true;
    entity.getComponent<xy::Callback>().userData = std::make_any<std::int32_t>(0);
    entity.getComponent<xy::Callback>().function = 
        [playArea](xy::Entity e, float dt) 
    {
        auto& state = std::any_cast<std::int32_t&>(e.getComponent<xy::Callback>().userData);
        auto& tx = e.getComponent<xy::Transform>();
        static float timer = 1.5f;
        switch (state)
        {
        case 0:
            //move down
            tx.move(0.f, -TitleSpeed * dt);
            if (tx.getPosition().y < (playArea.y / 2.f) + 8.f)
            {
                state = 1;
            }
            break;
        case 1:
            //hold
            timer -= dt;
            if (timer < 0)
            {
                timer = 1.5f;
                state = 2;
            }
            break;
        case 2:
            //exit
            tx.move(-TitleSpeed * dt, 0.f);
            if (tx.getPosition().x < -(xy::DefaultSceneSize.x - playArea.x) / 4.f)
            {
                state = 0;
                tx.setPosition(playArea.x / 2.f, (xy::DefaultSceneSize.y / 2.f) + 1.f);
                e.getComponent<xy::Callback>().active = false;
            }
            break;
        }
    };
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
}