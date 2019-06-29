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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/detail/Operators.hpp>

namespace
{
#include "tilemap.inl"
}

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State         (ss, ctx),
    m_backgroundScene   (ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildBackground();
    buildMenu();

    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

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

    m_backgroundScene.forwardEvent(evt);

    return true;
}

void MenuState::handleMessage(const xy::Message& msg)
{
    m_backgroundScene.forwardMessage(msg);
}

bool MenuState::update(float dt)
{
    m_backgroundScene.update(dt);
    return true;
}

void MenuState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_backgroundScene);
}

xy::StateID MenuState::stateID() const
{
    return StateID::MainMenu;
}

//private
void MenuState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_backgroundScene.addSystem<xy::CameraSystem>(mb);
    m_backgroundScene.addSystem<xy::RenderSystem>(mb);
}

void MenuState::loadResources()
{
    m_shaders.preload(ShaderID::TileMap, tilemapFrag, sf::Shader::Fragment);
}

void MenuState::buildBackground()
{
    //TODO check player/game progress and load background correspondingly
    if (m_mapLoader.load("menu.tmx"))
    //m_mapLoader.load("menu.tmx");
    {
        const auto& layers = m_mapLoader.getLayers();

        //for each layer create a drawable in the scene
        std::int32_t startDepth = -50;
        for (const auto& layer : layers)
        {
            auto entity = m_backgroundScene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Drawable>().setDepth(startDepth++);
            entity.getComponent<xy::Drawable>().setTexture(layer.indexMap);
            entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::TileMap));
            entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_indexMap");
            entity.getComponent<xy::Drawable>().bindUniform("u_tileSet", *layer.tileSet);
            entity.getComponent<xy::Drawable>().bindUniform("u_indexSize", sf::Vector2f(layer.indexMap->getSize()));
            entity.getComponent<xy::Drawable>().bindUniform("u_tileCount", layer.tileSetSize);

            auto& verts = entity.getComponent<xy::Drawable>().getVertices();
            sf::Vector2f texCoords(layer.indexMap->getSize());

            verts.emplace_back(sf::Vector2f(), sf::Vector2f());
            verts.emplace_back(sf::Vector2f(layer.layerSize.x, 0.f), sf::Vector2f(texCoords.x, 0.f));
            verts.emplace_back(layer.layerSize, texCoords);
            verts.emplace_back(sf::Vector2f(0.f, layer.layerSize.y), sf::Vector2f(0.f, texCoords.y));

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Transform>().setScale(xy::DefaultSceneSize / layer.layerSize);
        }
    }
}

void MenuState::buildMenu()
{

}