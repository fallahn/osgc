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

#include "MainState.hpp"
#include "StateIDs.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>

MainState::MainState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State (ss, ctx),
    m_scene     (ctx.appInstance.getMessageBus())
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

    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::SpriteAnimator>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
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
    
    auto rootEnt = m_scene.createEntity();
    rootEnt.addComponent<xy::Transform>().setScale(2.f, 2.f);
    rootEnt.getComponent<xy::Transform>().setPosition((xy::DefaultSceneSize.x - playArea.x) / 4.f, 60.f);

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-10);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::Background]));
    rootEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(320.f, 445.f); //TODO constify
    entity.addComponent<xy::Drawable>().setDepth(1);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::GunMount]));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    rootEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    auto mountEntity = entity;

    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(mountEntity.getComponent<xy::Transform>().getOrigin());
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textures[TextureID::RayGun]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(11.f, bounds.height / 2.f);
    mountEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    m_playerInput.setPlayerEntity(entity);
}