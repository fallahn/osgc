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

#include "GameState.hpp"
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "CommandIDs.hpp"
#include "Collision.hpp"
#include "NinjaDirector.hpp"
#include "Player.hpp"
#include "CameraTarget.hpp"
#include "NinjaStarSystem.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Rectangle.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
#include "tilemap.inl"
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_gameScene     (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_playerInput   (sd.inputBinding)
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildWorld();

    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());

    quitLoadingScreen();
}

//public
bool GameState::handleEvent(const sf::Event& evt)
{
    m_playerInput.handleEvent(evt);
    m_gameScene.forwardEvent(evt);
    return true;
}

void GameState::handleMessage(const xy::Message& msg)
{
    m_gameScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
    m_playerInput.update();
    m_gameScene.update(dt);
    return true;
}

void GameState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_gameScene);
}

//private
void GameState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<PlayerSystem>(mb);
    m_gameScene.addSystem<NinjaStarSystem>(mb);
    m_gameScene.addSystem<CameraTargetSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);

    m_gameScene.addDirector<NinjaDirector>(m_sprites);
}

void GameState::loadResources() 
{
    m_textureIDs[TextureID::Game::Background] = m_resources.load<sf::Texture>("assets/images/gearboy/background.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Background]).setRepeated(true);

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/gearboy/player.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Player] = spriteSheet.getSprite("player");

    m_playerAnimations[AnimID::Player::Idle] = spriteSheet.getAnimationIndex("idle", "player");
    m_playerAnimations[AnimID::Player::Jump] = spriteSheet.getAnimationIndex("jump", "player");
    m_playerAnimations[AnimID::Player::Run] = spriteSheet.getAnimationIndex("run", "player");

    spriteSheet.loadFromFile("assets/sprites/gearboy/star.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Star] = spriteSheet.getSprite("star");

    spriteSheet.loadFromFile("assets/sprites/gearboy/smoke_puff.spt", m_resources);
    m_sprites[SpriteID::GearBoy::SmokePuff] = spriteSheet.getSprite("smoke_puff");

    m_shaders.preload(ShaderID::TileMap, tilemapFrag, sf::Shader::Fragment);
}

void GameState::buildWorld()
{
    if (m_mapLoader.load("gb01.tmx"))
    {
        const float scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();
        m_gameScene.getDirector<NinjaDirector>().setSpriteScale(scale);

        //parallax background
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(-xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().setScale(scale, scale);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth);
        entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Background]));

        sf::Vector2f texSize(entity.getComponent<xy::Drawable>().getTexture()->getSize());
        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        verts.resize(8);
        //TODO find a way to read these values from the texture
        const float backgroundHeight = GameConst::VerticalTileCount * m_mapLoader.getTileSize();
        verts[1] = { sf::Vector2f(texSize.x, 0.f), sf::Vector2f(texSize.x, 0.f) };
        verts[2] = { sf::Vector2f(texSize.x, backgroundHeight), sf::Vector2f(texSize.x, backgroundHeight) };
        verts[3] = { sf::Vector2f(0.f, backgroundHeight ), sf::Vector2f(0.f, backgroundHeight) };

        texSize.y -= backgroundHeight;
        verts[4] = { sf::Vector2f(), sf::Vector2f(0.f, backgroundHeight) };
        verts[5] = { sf::Vector2f(texSize.x, 0.f), sf::Vector2f(texSize.x, backgroundHeight) };
        verts[6] = { texSize, sf::Vector2f(texSize.x, texSize.y + backgroundHeight) };
        verts[7] = { sf::Vector2f(0.f, texSize.y), sf::Vector2f(0.f, texSize.y + backgroundHeight) };

        entity.getComponent<xy::Drawable>().updateLocalBounds();
        //performs texture scrolling
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function =
            [texSize](xy::Entity e, float)
        {
            auto pos = e.getComponent<xy::Transform>().getWorldPosition();
            pos *= 0.05f;
            auto& verts = e.getComponent<xy::Drawable>().getVertices();
            verts[4].texCoords.x = pos.x;
            verts[5].texCoords.x = pos.x + texSize.x;
            verts[6].texCoords.x = pos.x + texSize.x;
            verts[7].texCoords.x = pos.x;

            pos *= 0.5f;
            verts[0].texCoords.x = pos.x;
            verts[1].texCoords.x = pos.x + texSize.x;
            verts[2].texCoords.x = pos.x + texSize.x;
            verts[3].texCoords.x = pos.x;
        };

        m_gameScene.getActiveCamera().getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        //map layers
        const auto& layers = m_mapLoader.getLayers();

        //for each layer create a drawable in the scene
        std::int32_t startDepth = GameConst::BackgroundDepth + 2;
        for (const auto& layer : layers)
        {
            auto entity = m_gameScene.createEntity();
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
            entity.getComponent<xy::Transform>().setScale(scale, scale);
        }

        //TODO read spawn point from map
        //create player
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().setScale(scale, scale);
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Player];
        entity.addComponent<xy::SpriteAnimation>().play(0);
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);

        auto& collision = entity.addComponent<CollisionBody>();
        collision.shapes[0].aabb = GameConst::Gearboy::PlayerBounds;
        collision.shapes[0].type = CollisionShape::Player;
        collision.shapes[0].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapes[1].aabb = GameConst::Gearboy::PlayerFoot;
        collision.shapes[1].type = CollisionShape::Foot;
        collision.shapes[1].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapes[2].aabb = GameConst::Gearboy::PlayerLeftHand;
        collision.shapes[2].type = CollisionShape::LeftHand;
        collision.shapes[2].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapes[3].aabb = GameConst::Gearboy::PlayerRightHand;
        collision.shapes[3].type = CollisionShape::RightHand;
        collision.shapes[3].collisionFlags = CollisionGroup::PlayerFlags;

        collision.shapeCount = 4;

        auto aabb = xy::Util::Rectangle::combine(GameConst::Gearboy::PlayerBounds, GameConst::Gearboy::PlayerFoot);
        aabb = xy::Util::Rectangle::combine(GameConst::Gearboy::PlayerLeftHand, aabb);
        aabb = xy::Util::Rectangle::combine(GameConst::Gearboy::PlayerRightHand, aabb);

        entity.addComponent<xy::BroadphaseComponent>().setArea(aabb);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Player | CollisionShape::Foot | CollisionShape::LeftHand | CollisionShape::RightHand);
        entity.addComponent<Player>().animations = m_playerAnimations;
        m_playerInput.setPlayerEntity(entity);

        m_gameScene.getActiveCamera().addComponent<CameraTarget>().target = entity;
        m_gameScene.getSystem<CameraTargetSystem>().setBounds({ sf::Vector2f(), m_mapLoader.getMapSize() * scale });

        //map collision data
        const auto& collisionShapes = m_mapLoader.getCollisionShapes();

        for (const auto& shape : collisionShapes)
        {
            entity = m_gameScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(shape.aabb.left * scale, shape.aabb.top * scale);
            entity.addComponent<CollisionBody>().shapes[0] = shape;
            //convert from world to local
            entity.getComponent<CollisionBody>().shapes[0].aabb.left = 0;
            entity.getComponent<CollisionBody>().shapes[0].aabb.top = 0;
            entity.getComponent<CollisionBody>().shapes[0].aabb.width *= scale;
            entity.getComponent<CollisionBody>().shapes[0].aabb.height *= scale;
            entity.getComponent<CollisionBody>().shapeCount = 1;
            entity.addComponent<xy::BroadphaseComponent>().setArea(entity.getComponent<CollisionBody>().shapes[0].aabb);
            entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(shape.type);
        }
    }
}