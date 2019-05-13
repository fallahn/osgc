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
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

#include <SFML/Graphics/Font.hpp>

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
    m_scene.addSystem<xy::UISystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);
}

void MenuState::loadResources()
{
    FontID::handles[FontID::Default] = m_sharedData.resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");
}

void MenuState::buildMenu()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);

    //title
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(100.f, 80.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setCharacterSize(120);
    entity.getComponent<xy::Text>().setString("Space Racers");

    auto& uiSystem = m_scene.getSystem<xy::UISystem>();

    sf::Vector2f itemPosition(100.f, 540.f);
    static const float itemStride = 40.f;
    static const std::uint32_t CharSize(60);

    //start game
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Start Game");
    entity.getComponent<xy::Text>().setCharacterSize(CharSize);
    auto bounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    requestStackClear();
                    requestStackPush(StateID::Race);
                }
            });
    itemPosition.y += itemStride;

    //quit to browser
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Return to Browser");
    entity.getComponent<xy::Text>().setCharacterSize(CharSize);
    bounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    requestStackClear();
                    requestStackPush(StateID::ParentState);
                }
            });
    itemPosition.y += itemStride;

    //quit
    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(itemPosition);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Quit");
    entity.getComponent<xy::Text>().setCharacterSize(CharSize);
    bounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::App::quit();
                }
            });
    itemPosition.y += itemStride;
}