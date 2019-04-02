/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#include "MenuState.hpp"
#include "StateIDs.hpp"
#include "ResourceIDs.hpp"
#include "PluginExport.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_scene         (ctx.appInstance.getMessageBus())
{
    initScene();
    buildMenu();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == sf::Keyboard::Escape)
        {
            xy::App::quit();
        }
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

void MenuState::buildMenu()
{
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::HandDrawn]);
    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(120.f, 80.f);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Hello!");
    entity.getComponent<xy::Text>().setCharacterSize(80);
    entity.addComponent<xy::Drawable>();


    entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(120.f, 880.f);
    entity.addComponent<xy::Text>(font);
    entity.getComponent<xy::Text>().setString("Return to Launcher");
    entity.getComponent<xy::Text>().setCharacterSize(80);
    entity.addComponent<xy::Drawable>();

    auto textBounds = xy::Text::getLocalBounds(entity);
    entity.addComponent<xy::UIHitBox>().area = textBounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
            [&](xy::Entity e, sf::Uint64 flags)
    {
        if (flags & xy::UISystem::LeftMouse)
        {
            requestStackClear();
            requestStackPush(StateID::ParentState);
        }
    });
}