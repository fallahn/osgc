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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/BitmapText.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/BitmapTextSystem.hpp>

#include <xyginext/graphics/BitmapFont.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/core/FileSystem.hpp>

namespace
{
    const std::size_t MaxLevels = 100;
}

MainState::MainState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State     (ss, ctx),
    m_scene         (ctx.appInstance.getMessageBus(), 1024u)
{
    launchLoadingScreen();
    initScene();
    loadResources();

    buildArena();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    quitLoadingScreen();
}

//public
bool MainState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    m_playerInput.handleEvent(evt);
    m_scene.forwardEvent(evt);

    return true;
}

void MainState::handleMessage(const xy::Message& msg)
{
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
    m_scene.addSystem<xy::CommandSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::BitmapTextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    m_scene.addDirector<GameDirector>(m_nodeSet, m_bubbleSprites).loadLevelData();
}

void MainState::loadResources()
{
    xy::SpriteSheet spriteSheet;

    spriteSheet.loadFromFile("assets/sprites/red.spt", m_resources);
    m_bubbleSprites[BubbleID::Red] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/green.spt", m_resources);
    m_bubbleSprites[BubbleID::Green] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/blue.spt", m_resources);
    m_bubbleSprites[BubbleID::Blue] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/cyan.spt", m_resources);
    m_bubbleSprites[BubbleID::Cyan] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/magenta.spt", m_resources);
    m_bubbleSprites[BubbleID::Magenta] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/yellow.spt", m_resources);
    m_bubbleSprites[BubbleID::Yellow] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/orange.spt", m_resources);
    m_bubbleSprites[BubbleID::Orange] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/grey.spt", m_resources);
    m_bubbleSprites[BubbleID::Grey] = spriteSheet.getSprite("ball");

    spriteSheet.loadFromFile("assets/sprites/misc.spt", m_resources);
    m_sprites[SpriteID::GameOver] = spriteSheet.getSprite("game_over");
    m_sprites[SpriteID::Pause] = spriteSheet.getSprite("paused");
    m_sprites[SpriteID::Title] = spriteSheet.getSprite("title");
    m_sprites[SpriteID::TopBar] = spriteSheet.getSprite("top_bar");

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
    m_nodeSet.rootNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    m_nodeSet.gunNode = entity;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_nodeSet.gunNode.getComponent<xy::Transform>().getOrigin());
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
    entity.addComponent<xy::Transform>().setPosition(0.f, -m_sprites[SpriteID::TopBar].getTextureRect().height);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::TopBar];
    m_nodeSet.barNode.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());   

    m_scene.getDirector<GameDirector>().activateLevel();
}