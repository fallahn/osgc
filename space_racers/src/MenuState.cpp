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

#include "MenuState.hpp"
#include "StateIDs.hpp"
#include "ResourceIDs.hpp"
#include "PluginExport.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>

#include <SFML/Graphics/Font.hpp>

namespace
{
    const std::int32_t BackgroundDepth = -20;
    const std::int32_t MenuDepth = 0;
    const std::int32_t ButtonDepth = 1;
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildMenu();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    registerCommand("debug_mode",
        [&](const std::string&)
        {
            requestStackClear();
            requestStackPush(StateID::Debug);
        });

    quitLoadingScreen();
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    m_scene.getSystem<xy::UISystem>().handleEvent(evt);
    m_scene.forwardEvent(evt);

    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
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
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
}

void MenuState::loadResources()
{
    FontID::handles[FontID::Default] = m_sharedData.resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");

    TextureID::handles[TextureID::MainMenu] = m_resources.load<sf::Texture>("assets/images/menu_title.png");

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/menu_buttons.spt", m_resources);
    SpriteID::sprites[SpriteID::TimeTrialButton] = spriteSheet.getSprite("timetrial");
    SpriteID::sprites[SpriteID::LocalButton] = spriteSheet.getSprite("local");
    SpriteID::sprites[SpriteID::NetButton] = spriteSheet.getSprite("net");
    SpriteID::sprites[SpriteID::OptionsButton] = spriteSheet.getSprite("options");
    SpriteID::sprites[SpriteID::QuitButton] = spriteSheet.getSprite("quit");
}

void MenuState::buildMenu()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);

    //title
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setDepth(MenuDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::MainMenu));
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    auto& parentTx = entity.getComponent<xy::Transform>();


    auto& uiSystem = m_scene.getSystem<xy::UISystem>();
    auto mouseEnter = uiSystem.addMouseMoveCallback(
        [](xy::Entity e, sf::Vector2f)
        {
            auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
            bounds.left = bounds.width;
            e.getComponent<xy::Sprite>().setTextureRect(bounds);
        });
    auto mouseExit = uiSystem.addMouseMoveCallback([](xy::Entity e, sf::Vector2f) 
        {
            auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
            bounds.left = 0.f;
            e.getComponent<xy::Sprite>().setTextureRect(bounds);
        });


    sf::Vector2f itemPosition(402.f ,380.f);

    //time trial
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(ButtonDepth);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::TimeTrialButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {

                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());
    
    itemPosition.y += bounds.height;

    //local
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(ButtonDepth);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::LocalButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {

                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //net
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(ButtonDepth);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::NetButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    //TODO choose host/join
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;

    //options
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(ButtonDepth);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::OptionsButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Console::show();
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());

    itemPosition.y += bounds.height;


    //quit
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>().setDepth(ButtonDepth);
    entity.addComponent<xy::Sprite>() = SpriteID::sprites[SpriteID::QuitButton];
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    requestStackClear();
                    requestStackPush(StateID::ParentState);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;
    parentTx.addChild(entity.getComponent<xy::Transform>());
}