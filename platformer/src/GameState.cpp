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
#include "FluidAnimationSystem.hpp"
#include "MessageIDs.hpp"
#include "BobAnimationSystem.hpp"
#include "EnemySystem.hpp"

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
#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
#include "tilemap.inl"
#include "transition.inl"
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_gameScene     (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_playerInput   (sd.inputBinding),
    m_theme         ("gearboy")
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
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    m_playerInput.handleEvent(evt);
    m_gameScene.forwardEvent(evt);
    return true;
}

void GameState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.type == PlayerEvent::Exited)
        {
            m_sharedData.nextMap = m_mapLoader.getNextMap() + ".tmx";

            //copy the window to a texture so we can do wibbly things with it
            auto* window = xy::App::getRenderWindow();
            m_sharedData.transitionContext.texture.create(window->getSize().x, window->getSize().y);
            m_sharedData.transitionContext.texture.update(*window);
            m_sharedData.transitionContext.shader = &m_shaders.get(ShaderID::PixelTransition);

            requestStackPush(StateID::Transition);
        }
    }

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
    m_gameScene.addSystem<FluidAnimationSystem>(mb);
    m_gameScene.addSystem<BobAnimationSystem>(mb);
    m_gameScene.addSystem<EnemySystem>(mb);
    m_gameScene.addSystem<CameraTargetSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);

    m_gameScene.addDirector<NinjaDirector>(m_sprites);
}

void GameState::loadResources() 
{
    m_textureIDs[TextureID::Game::Background] = m_resources.load<sf::Texture>("assets/images/"+m_theme+"/background.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Background]).setRepeated(true);

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/"+m_theme+"/player.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Player] = spriteSheet.getSprite("player");

    m_playerAnimations[AnimID::Player::Idle] = spriteSheet.getAnimationIndex("idle", "player");
    m_playerAnimations[AnimID::Player::Jump] = spriteSheet.getAnimationIndex("jump", "player");
    m_playerAnimations[AnimID::Player::Run] = spriteSheet.getAnimationIndex("run", "player");
    m_playerAnimations[AnimID::Player::Die] = spriteSheet.getAnimationIndex("die", "player");

    spriteSheet.loadFromFile("assets/sprites/"+m_theme+"/projectile.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Star] = spriteSheet.getSprite("projectile");

    spriteSheet.loadFromFile("assets/sprites/"+m_theme+"/smoke_puff.spt", m_resources);
    m_sprites[SpriteID::GearBoy::SmokePuff] = spriteSheet.getSprite("smoke_puff");

    spriteSheet.loadFromFile("assets/sprites/"+m_theme+"/checkpoint.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Checkpoint] = spriteSheet.getSprite("checkpoint");
    m_checkpointAnimations[AnimID::Checkpoint::Idle] = spriteSheet.getAnimationIndex("idle", "checkpoint");
    m_checkpointAnimations[AnimID::Checkpoint::Activate] = spriteSheet.getAnimationIndex("activate", "checkpoint");

    spriteSheet.loadFromFile("assets/sprites/"+m_theme+"/lava.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Lava] = spriteSheet.getSprite("lava");
    const_cast<sf::Texture*>(m_sprites[SpriteID::GearBoy::Lava].getTexture())->setRepeated(true); //uuugghhhhhh

    spriteSheet.loadFromFile("assets/sprites/"+m_theme+"/water.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Water] = spriteSheet.getSprite("water");
    const_cast<sf::Texture*>(m_sprites[SpriteID::GearBoy::Water].getTexture())->setRepeated(true);

    spriteSheet.loadFromFile("assets/sprites/"+m_theme+"/collectibles.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Coin] = spriteSheet.getSprite("coin");
    m_sprites[SpriteID::GearBoy::Shield] = spriteSheet.getSprite("shield");
    m_sprites[SpriteID::GearBoy::Ammo] = spriteSheet.getSprite("ammo");

    spriteSheet.loadFromFile("assets/sprites/" + m_theme + "/enemies.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Bird] = spriteSheet.getSprite("bird");
    m_sprites[SpriteID::GearBoy::Walker] = spriteSheet.getSprite("walker");
    m_sprites[SpriteID::GearBoy::Crawler] = spriteSheet.getSprite("crawler");

    m_shaders.preload(ShaderID::TileMap, tilemapFrag2, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::PixelTransition, PixelateFrag, sf::Shader::Fragment);
}

void GameState::buildWorld()
{
    if (m_mapLoader.load(m_sharedData.nextMap))
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
            entity.getComponent<xy::Drawable>().bindUniform("u_tileSize", sf::Vector2f(m_mapLoader.getTileSize(), m_mapLoader.getTileSize()));

            auto& verts = entity.getComponent<xy::Drawable>().getVertices();
            sf::Vector2f texCoords(layer.indexMap->getSize());
            //texCoords *= m_mapLoader.getTileSize();

            verts.emplace_back(sf::Vector2f(), sf::Vector2f());
            verts.emplace_back(sf::Vector2f(layer.layerSize.x, 0.f), sf::Vector2f(texCoords.x, 0.f));
            verts.emplace_back(layer.layerSize, texCoords);
            verts.emplace_back(sf::Vector2f(0.f, layer.layerSize.y), sf::Vector2f(0.f, texCoords.y));

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Transform>().setScale(scale, scale);
        }

        //create player
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(m_mapLoader.getSpawnPoint() * scale);
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

            if (shape.ID > -1)
            {
                switch (shape.type)
                {
                default: break;
                case CollisionShape::Fluid:
                {
                    sf::Vector2f size(shape.aabb.width, shape.aabb.height);
                    size *= scale;

                    entity.addComponent<xy::Drawable>().setDepth(-1);
                    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
                    verts.emplace_back(sf::Vector2f());
                    verts.emplace_back(sf::Vector2f(size.x, 0.f), sf::Vector2f(size.x, 0.f));
                    verts.emplace_back(size, size);
                    verts.emplace_back(sf::Vector2f(0.f, size.y), sf::Vector2f(0.f, size.y));

                    entity.getComponent<xy::Drawable>().updateLocalBounds();
                    std::int32_t spriteID;
                    if (shape.ID == 0)
                    {
                        spriteID = SpriteID::GearBoy::Lava;
                    }
                    else
                    {
                        spriteID = SpriteID::GearBoy::Water;
                    }
                    entity.getComponent<xy::Drawable>().setTexture(m_sprites[spriteID].getTexture());
                    entity.addComponent<Fluid>().frameHeight = m_sprites[spriteID].getTextureBounds().height;
                    if (m_sprites[spriteID].getAnimations()[0].framerate != 0)
                    {
                        entity.getComponent<Fluid>().frameTime = 1.f / m_sprites[spriteID].getAnimations()[0].framerate;
                    }
                }
                break;
                case CollisionShape::Checkpoint:
                    entity.addComponent<xy::CommandTarget>().ID = CommandID::World::CheckPoint;

                    if (shape.ID > 0)
                    {
                        auto checkpointEnt = m_gameScene.createEntity();
                        checkpointEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getPosition());
                        checkpointEnt.getComponent<xy::Transform>().setScale(scale, scale);
                        checkpointEnt.addComponent<xy::Drawable>().setDepth(-1);
                        checkpointEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Checkpoint];
                        checkpointEnt.addComponent<xy::SpriteAnimation>().play(m_checkpointAnimations[AnimID::Checkpoint::Idle]);

                        entity.addComponent<xy::Callback>().function =
                            [&,checkpointEnt](xy::Entity e, float) mutable
                        {
                            if (checkpointEnt.getComponent<xy::SpriteAnimation>().getAnimationIndex() ==
                                m_checkpointAnimations[AnimID::Checkpoint::Idle])
                            {
                                checkpointEnt.getComponent<xy::SpriteAnimation>().play(m_checkpointAnimations[AnimID::Checkpoint::Activate]);
                            }
                            e.getComponent<xy::Callback>().active = false;
                        };
                    }

                    break;
                case CollisionShape::Collectible:
                {
                    std::int32_t spriteID = 0;
                    switch (shape.ID)
                    {
                    default:
                    case 0:
                        spriteID = SpriteID::GearBoy::Coin;
                        break;
                    case 1:
                        spriteID = SpriteID::GearBoy::Shield;
                        break;
                    case 2:
                        spriteID = SpriteID::GearBoy::Ammo;
                        break;
                    }

                    auto cEnt = m_gameScene.createEntity();
                    cEnt.addComponent<xy::Transform>().setScale(scale, scale);
                    cEnt.addComponent<xy::Drawable>();
                    cEnt.addComponent<xy::Sprite>() = m_sprites[spriteID];
                    cEnt.addComponent<xy::SpriteAnimation>().play(0);
                    cEnt.addComponent<BobAnimation>().parent = entity;
                    entity.getComponent<xy::Transform>().addChild(cEnt.getComponent<xy::Transform>());
                }
                    break;
                }
            }
        }

        loadEnemies();
    }
    else
    {
        //TODO create an error message state
        requestStackClear();
        requestStackPush(StateID::MainMenu);
    }
}

void GameState::loadEnemies()
{
    const float scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();

    const auto& enemies = m_mapLoader.getEnemySpawns();
    for (auto& [path, id] : enemies)
    {
        auto position = (path.second * scale) - (path.first * scale);
        position /= 2.f;
        position += (path.first * scale);
        
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position);
        entity.getComponent<xy::Transform>().setScale(scale, scale);
        entity.addComponent<xy::Drawable>();
        
        switch (id)
        {
        default:
        case 0:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Crawler];
            entity.addComponent<Enemy>().type = Enemy::Crawler;
            break;
        case 1:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Bird];
            entity.addComponent<Enemy>().type = Enemy::Bird;
            break;
        case 2:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Walker];
            entity.addComponent<Enemy>().type = Enemy::Walker;
            break;
        }
        entity.getComponent<Enemy>().start = path.first * scale;
        entity.getComponent<Enemy>().end = path.second * scale;

        entity.addComponent<xy::SpriteAnimation>().play(0);

        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

        bounds.left -= bounds.width / 2.f;
        bounds.top -= bounds.height / 2.f;

        auto& collision = entity.addComponent<CollisionBody>();
        collision.shapes[0].aabb = bounds;
        collision.shapes[0].type = CollisionShape::Enemy;
        collision.shapes[0].collisionFlags = CollisionShape::Player;

        entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Enemy);
    }
}