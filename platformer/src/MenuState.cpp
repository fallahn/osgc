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
#include "GameConsts.hpp"
#include "ShapeUtils.hpp"
#include "Collision.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/gui/Gui.hpp>
#include <xyginext/detail/Operators.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Rectangle.hpp>

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

    m_backgroundScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_backgroundScene.addSystem<xy::SpriteSystem>(mb);
    m_backgroundScene.addSystem<xy::CameraSystem>(mb);
    m_backgroundScene.addSystem<xy::RenderSystem>(mb);
}

void MenuState::loadResources()
{
    m_textureIDs[TextureID::Menu::Background] = m_resources.load<sf::Texture>("assets/images/gearboy/background.png");

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/gearboy/player.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Player] = spriteSheet.getSprite("player");

    spriteSheet.loadFromFile("assets/sprites/gearboy/star.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Star] = spriteSheet.getSprite("star");

    m_shaders.preload(ShaderID::TileMap, tilemapFrag, sf::Shader::Fragment);
}

void MenuState::buildBackground()
{
    if (m_mapLoader.load("menu.tmx"))
    {
        const float pixelScale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();
        const auto& layers = m_mapLoader.getLayers();

        //for each layer create a drawable in the scene
        std::int32_t startDepth = GameConst::BackgroundDepth + 2;
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

        //background
        auto entity = m_backgroundScene.createEntity();
        entity.addComponent<xy::Transform>().setScale(pixelScale,pixelScale);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::Background]));

        entity = m_backgroundScene.createEntity();
        entity.addComponent<xy::Transform>().setScale(pixelScale, pixelScale);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Menu::Background]));
        entity.getComponent<xy::Sprite>().setTextureRect({ 0.f, 272.f, 480.f, 82.f });

        //player
        entity = m_backgroundScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().setScale(pixelScale, pixelScale);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Player];
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);
        
        auto& collision = entity.addComponent<CollisionBody>();
        collision.shapes[0].aabb = GameConst::Gearboy::PlayerBounds;
        collision.shapes[0].type = CollisionShape::Player;
        collision.shapes[0].collisionFlags = CollisionShape::Solid | CollisionShape::Water;

        collision.shapes[1].aabb = GameConst::Gearboy::PlayerFoot;
        collision.shapes[1].type = CollisionShape::Sensor;
        collision.shapes[1].collisionFlags = CollisionShape::Solid | CollisionShape::Water;
        collision.shapeCount = 2;
        entity.addComponent<xy::BroadphaseComponent>().setArea(xy::Util::Rectangle::combine(GameConst::Gearboy::PlayerBounds, GameConst::Gearboy::PlayerFoot));
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Player | CollisionShape::Sensor);

#ifdef XY_DEBUG
        //player debug bounds
        auto debugEnt = m_backgroundScene.createEntity();
        debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        debugEnt.getComponent<xy::Transform>().move(GameConst::Gearboy::PlayerBounds.left, GameConst::Gearboy::PlayerBounds.top);
        Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { GameConst::Gearboy::PlayerBounds.width, GameConst::Gearboy::PlayerBounds.height }, sf::Color::Red);
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());

        debugEnt = m_backgroundScene.createEntity();
        debugEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        debugEnt.getComponent<xy::Transform>().move(GameConst::Gearboy::PlayerFoot.left, GameConst::Gearboy::PlayerFoot.top);
        Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { GameConst::Gearboy::PlayerFoot.width, GameConst::Gearboy::PlayerFoot.height }, sf::Color::Blue);
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());
#endif //XY_DEBUG


        const auto& collisionShapes = m_mapLoader.getCollisionShapes();

        for (const auto& shape : collisionShapes)
        {
            entity = m_backgroundScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(shape.aabb.left * pixelScale, shape.aabb.top * pixelScale);
            entity.getComponent<xy::Transform>().setScale(pixelScale, pixelScale);
            entity.addComponent<CollisionBody>().shapes[0] = shape;
            //convert from world to local
            entity.getComponent<CollisionBody>().shapes[0].aabb.left = 0;
            entity.getComponent<CollisionBody>().shapes[0].aabb.top = 0;
            entity.getComponent<CollisionBody>().shapeCount = 1;
            entity.addComponent<xy::BroadphaseComponent>().setArea(entity.getComponent<CollisionBody>().shapes[0].aabb);
            entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(shape.type);

#ifdef XY_DEBUG
            debugEnt = m_backgroundScene.createEntity();
            debugEnt.addComponent<xy::Transform>();
            Shape::setRectangle(debugEnt.addComponent<xy::Drawable>(), { shape.aabb.width, shape.aabb.height }, sf::Color::Red);
            entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());
#endif //XY_DEBUG
        }
    }
}

void MenuState::buildMenu()
{

}