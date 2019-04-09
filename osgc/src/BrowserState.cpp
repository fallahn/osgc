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

#include "BrowserState.hpp"
#include "States.hpp"
#include "ResourceIDs.hpp"
#include "Game.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Font.hpp>

BrowserState::BrowserState(xy::StateStack& ss, xy::State::Context ctx, Game& game)
    : xy::State     (ss,ctx),
    m_gameInstance  (game),
    m_scene         (ctx.appInstance.getMessageBus())
{
    //make sure to unload any active plugin so our asset directory is correct
    game.unloadPlugin();
    
    createScene();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    ctx.appInstance.setMouseCursorVisible(true);
}

bool BrowserState::handleEvent(const sf::Event& evt)
{
    /*if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == sf::Keyboard::Space)
        {
            m_gameInstance.loadPlugin("plugins/shooter");
        }
    }*/

    m_scene.getSystem<xy::UISystem>().handleEvent(evt);
    m_scene.forwardEvent(evt);
    return true;
}

void BrowserState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool BrowserState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void BrowserState::draw()
{
    auto rw = getContext().appInstance.getRenderWindow();
    rw->draw(m_scene);
}

xy::StateID BrowserState::stateID() const
{
    return States::BrowserState;
}

void BrowserState::createScene()
{
    //add the systems
    auto& messageBus = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::TextSystem>(messageBus);
    m_scene.addSystem<xy::UISystem>(messageBus);
    m_scene.addSystem<xy::RenderSystem>(messageBus);

    //load resources
    FontID::handles[FontID::MenuFont] = m_resources.load<sf::Font>("assets/fonts/ProggyClean.ttf");

    sf::Vector2f currentPos(20.f, 120.f);
    auto& menuFont = m_resources.get<sf::Font>(FontID::handles[FontID::MenuFont]);

    auto pluginList = xy::FileSystem::listDirectories("plugins");
    for (const auto& dir : pluginList)
    {
        //TODO search for plugin info file and only load if can be validated
        auto entity = m_scene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(currentPos);
        entity.addComponent<xy::Text>(menuFont).setString(dir);
        entity.getComponent<xy::Text>().setCharacterSize(30);
        entity.addComponent<xy::Drawable>();

        auto textBounds = xy::Text::getLocalBounds(entity);
        entity.addComponent<xy::UIHitBox>().area = textBounds;
        entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
            m_scene.getSystem<xy::UISystem>().addMouseButtonCallback(
                [&, dir](xy::Entity, sf::Uint64 flags)
        {
            if (flags & xy::UISystem::LeftMouse)
            {
                m_gameInstance.loadPlugin("plugins/" + dir);
            }
        });

        currentPos.y += textBounds.height * 1.1f;
    }
}









