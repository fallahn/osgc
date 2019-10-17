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
#include "ShapeUtils.hpp"
#include "PhysicsSystem.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/util/Vector.hpp>

#include <chipmunk/chipmunk.h>

#include <SFML/Window/Event.hpp>

namespace
{
    //temp until we get a proper controller going
    xy::Entity player;
    const float PlayerStrength = 320.f;
    const float MaxVelocity = 800.f;
    const float MaxVelSqr = MaxVelocity * MaxVelocity;
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State(ss, ctx),
    m_gameScene(ctx.appInstance.getMessageBus())
{
    initScene();
    buildArena();
}

//public
bool MenuState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::ui::wantsKeyboard() || xy::ui::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased
        && evt.key.code == sf::Keyboard::Escape)
    {
        xy::App::quit();
    }

    m_gameScene.forwardEvent(evt);
    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    m_gameScene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
    //temp until we move this to a controller
    if (player.isValid())
    {
        sf::Vector2f movement;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {
            movement.x -= 1.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {
            movement.x += 1.f;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        {
            movement.y -= 1.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        {
            movement.y += 1.f;
        }
        if (xy::Util::Vector::lengthSquared(movement) > 1)
        {
            movement = xy::Util::Vector::normalise(movement);
        }

        movement *= PlayerStrength;
        
        auto& vel = player.getComponent<sf::Vector2f>();
        if (xy::Util::Vector::lengthSquared(vel) < MaxVelSqr)
        {
            vel += movement;
        }
    }

    m_gameScene.update(dt);
    return true;
}

void MenuState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_gameScene);
}

xy::StateID MenuState::stateID() const
{
    return StateID::MainMenu;
}

//private
void MenuState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<PhysicsSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);

    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setView(getContext().defaultView.getSize());
    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setViewport(getContext().defaultView.getViewport());
}

void MenuState::buildArena()
{
    auto& physWorld = m_gameScene.getSystem<PhysicsSystem>();
    ShapeProperties properties;
    properties.elasticity = 0.6f;

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<PhysicsObject>() = physWorld.createObject({}, PhysicsObject::Type::Static);
    entity.getComponent<PhysicsObject>().addLineShape(properties, { 0.f, xy::DefaultSceneSize.y - 40.f }, { xy::DefaultSceneSize.x, xy::DefaultSceneSize.y - 40.f });
    entity.getComponent<PhysicsObject>().addLineShape(properties, { 0.f, 0.f }, { xy::DefaultSceneSize.x, 0.f });
    entity.getComponent<PhysicsObject>().addLineShape(properties, { 0.f, 0.f }, { 0.f, xy::DefaultSceneSize.y - 40.f });
    entity.getComponent<PhysicsObject>().addLineShape(properties, { xy::DefaultSceneSize.x, 0.f }, { xy::DefaultSceneSize.x, xy::DefaultSceneSize.y - 40.f });

    //floor graphic
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y - 40.f);
    Shape::setRectangle(entity.addComponent<xy::Drawable>(), { xy::DefaultSceneSize.x, 40.f });


    //ball
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<PhysicsObject>() = physWorld.createObject({ xy::DefaultSceneSize.x / 2.f, 560.f });
    entity.getComponent<PhysicsObject>().addCircleShape(properties, 45.f);
    Shape::setCircle(entity.addComponent<xy::Drawable>(), 45.f);

    //tower
    const sf::Vector2f position(1400.f, 700.f);
    const sf::Vector2f size(80.f, 38.f);
    const sf::Vector2f padding(4.f, 2.f);
    const std::vector<sf::Vector2f> box =
    {
        -size / 2.f,
        sf::Vector2f(size.x / 2.f, -size.y / 2.f),
        sf::Vector2f(size.x / 2.f, size.y / 2.f),
        sf::Vector2f(-size.x / 2.f, size.y / 2.f),
        -size / 2.f,
    };
    for (auto y = 0; y < 10; ++y)
    {
        for (auto x = 0; x < 3; ++x)
        {
            auto pos = position;
            pos.x += (x * (size.x + padding.x));
            pos.y += (y * (size.y + padding.y));

            entity = m_gameScene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<PhysicsObject>() = physWorld.createObject(pos);
            entity.getComponent<PhysicsObject>().addBoxShape(properties, size);
            Shape::setPolyLine(entity.addComponent<xy::Drawable>(), box);
        }
    }

    addHand();
}

void MenuState::addHand()
{
    ShapeProperties properties;
    properties.elasticity = 0.4f;
    properties.mass = 2.5f;

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<PhysicsObject>() = m_gameScene.getSystem<PhysicsSystem>().createObject({ 400.f, xy::DefaultSceneSize.y / 2.f }, PhysicsObject::Type::Kinematic);
    entity.getComponent<PhysicsObject>().addCircleShape(properties, 120.f);
    Shape::setCircle(entity.addComponent<xy::Drawable>(), 120.f, sf::Color::Red);
    entity.addComponent<sf::Vector2f>();
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [](xy::Entity e, float)
    {
        auto& vel = e.getComponent<sf::Vector2f>();
        vel *= 0.9f;
        e.getComponent<PhysicsObject>().setVelocity(vel);
    };

    player = entity;
}