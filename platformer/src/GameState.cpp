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
#include "ShieldAnimationSystem.hpp"
#include "UIDirector.hpp"
#include "SoundEffectsDirector.hpp"
#include "MovingPlatform.hpp"
#include "CrateSystem.hpp"
#include "Camera3D.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/ParticleSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Rectangle.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/detail/Operators.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>

namespace
{
#include "tilemap.inl"
#include "transition.inl"
#include "3dRender.inl"

    std::size_t fontID = 0;
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_tilemapScene  (ctx.appInstance.getMessageBus()),
    m_gameScene     (ctx.appInstance.getMessageBus(), 1024),
    m_uiScene       (ctx.appInstance.getMessageBus()),
    m_sharedData    (sd),
    m_playerInput   (sd.inputBinding),
    m_ambience      (m_audioResource),
    m_effects       (m_audioResource)
{
    launchLoadingScreen();
    //sd.theme = "mes";
    //sd.nextMap = "mes01.tmx";
    initScene();
    loadResources();
    buildWorld();
    buildUI();

    m_tilemapScene.getActiveCamera().getComponent<xy::Camera>().setView(xy::DefaultSceneSize);
    m_tilemapScene.getActiveCamera().getComponent<xy::Camera>().setViewport({ 0.f, 0.f, 1.f, 1.f });
    auto& cam3D = m_tilemapScene.getActiveCamera().addComponent<Camera3D>();
    auto fov = cam3D.calcFOV(xy::DefaultSceneSize.y);
    cam3D.projectionMatrix = glm::perspective(fov, xy::DefaultSceneSize.x / xy::DefaultSceneSize.y, 700.f, 1900.f);

    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_gameScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
    m_gameScene.getActiveListener().getComponent<xy::AudioListener>().setDepth(Camera3D::defaultDepth);

    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
    
    quitLoadingScreen();
}

//public
bool GameState::handleEvent(const sf::Event& evt)
{
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::P:
        case sf::Keyboard::Pause:
        case sf::Keyboard::Escape:
            requestStackPush(StateID::Pause);
            break;
#ifdef XY_DEBUG
        case sf::Keyboard::L:
            requestStackPush(StateID::Dialogue);
            break;
        case sf::Keyboard::R:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::World::CheckPoint;
            cmd.action = [&](xy::Entity e, float)
            {
                if (e.getComponent<CollisionBody>().shapes[0].ID == 0)
                {
                    auto pos = e.getComponent<xy::Transform>().getPosition();
                    xy::Command cmd2;
                    cmd2.targetFlags = CommandID::World::Player;
                    cmd2.action = [&, pos](xy::Entity f, float)
                    {
                        f.getComponent<xy::Transform>().setPosition(pos);
                    };
                    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd2);
                }
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
            break;
#endif
        }
    }
    else if (evt.type == sf::Event::JoystickButtonReleased)
    {
        switch (evt.joystickButton.button)
        {
        default: break;
        case 7:
            requestStackPush(StateID::Pause);
            break;
        }
    }

    m_playerInput.handleEvent(evt);
    m_tilemapScene.forwardEvent(evt);
    m_gameScene.forwardEvent(evt);
    m_uiScene.forwardEvent(evt);
    return true;
}

void GameState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.type == PlayerEvent::Exited
            && getStackSize() == 1) //don't do this if already transitioning
        {
            m_sharedData.nextMap = m_mapLoader.getNextMap() + ".tmx";
            m_sharedData.theme = m_mapLoader.getNextTheme();
            m_sharedData.saveProgress();

            //copy the window to a texture so we can do wibbly things with it
            auto* window = xy::App::getRenderWindow();
            m_sharedData.transitionContext.texture.create(window->getSize().x, window->getSize().y);
            m_sharedData.transitionContext.texture.update(*window);

            if (data.id == 1)
            {
                //end of a stage
                m_sharedData.transitionContext.shader = &m_shaders.get(ShaderID::NoiseTransition);
                m_sharedData.transitionContext.nextState = StateID::Summary;
            }
            else
            {
                m_sharedData.transitionContext.shader = &m_shaders.get(ShaderID::PixelTransition);
                m_sharedData.transitionContext.nextState = StateID::Game;

                auto& emitter = m_gameScene.getActiveCamera().getComponent<xy::AudioEmitter>();
                if (emitter.getStatus() == xy::AudioEmitter::Stopped)
                {
                    emitter.play();
                }
            }
            requestStackPush(StateID::Transition);
        }
        else if (data.type == PlayerEvent::TriggerDialogue)
        {
            m_playerInput.setEnabled(false);
            requestStackPush(StateID::Dialogue);
        }
    }
    else if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        switch (data.type)
        {
        default: break;
        case GameEvent::TimerExpired:
        case GameEvent::LivesExpired:
            auto* window = xy::App::getRenderWindow();
            m_sharedData.transitionContext.texture.create(window->getSize().x, window->getSize().y);
            m_sharedData.transitionContext.texture.update(*window);
            m_sharedData.transitionContext.shader = &m_shaders.get(ShaderID::PixelTransition);

            m_sharedData.menuID = MenuID::GameOver;
            requestStackPush(StateID::MenuConfirm);
            break;
        }
    }
    else if (msg.id == xy::Message::StateMessage)
    {
        const auto& data = msg.getData<xy::Message::StateEvent>();
        if (data.type == xy::Message::StateEvent::Popped
            && data.id == StateID::Dialogue)
        {
            m_playerInput.setEnabled(true);
        }
    }
    m_tilemapScene.forwardMessage(msg);
    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
    m_playerInput.update();
    m_gameScene.update(dt);
    m_uiScene.update(dt);

    //do this after game scene update so that the view is up to date and
    //not lagging behind
    auto pos = m_gameScene.getActiveCamera().getComponent<xy::Transform>().getPosition();
    m_tilemapScene.getActiveCamera().getComponent<xy::Transform>().setPosition(pos);
    m_tilemapScene.update(dt);
    
    //the camera projection should be the same for both scenes so we can save here
    //by only updating the shader uniforms once
    const auto& mat = m_tilemapScene.getActiveCamera().getComponent<Camera3D>().viewProjectionMatrix;
    m_shaders.get(ShaderID::TileMap3D).setUniform("u_viewProjectionMatrix", sf::Glsl::Mat4(&mat[0][0]));

    return true;
}

void GameState::draw()
{
    m_tilemapBuffer.clear(sf::Color::Transparent);
    m_tilemapBuffer.draw(m_tilemapScene);
    m_tilemapBuffer.display();

    auto& rw = getContext().renderWindow;
    rw.draw(m_gameScene);
    rw.draw(m_uiScene);
}

//private
void GameState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_tilemapScene.addSystem<xy::SpriteSystem>(mb);
    m_tilemapScene.addSystem<FluidAnimationSystem>(mb);
    m_tilemapScene.addSystem<xy::SpriteAnimator>(mb);
    m_tilemapScene.addSystem<Camera3DSystem>(mb);
    m_tilemapScene.addSystem<xy::CameraSystem>(mb);
    m_tilemapScene.addSystem<xy::RenderSystem>(mb);

    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<CrateSystem>(mb, m_sharedData);
    m_gameScene.addSystem<PlayerSystem>(mb, m_sharedData);
    m_gameScene.addSystem<ShieldAnimationSystem>(mb);
    m_gameScene.addSystem<NinjaStarSystem>(mb);
    m_gameScene.addSystem<FluidAnimationSystem>(mb);
    m_gameScene.addSystem<BobAnimationSystem>(mb);
    m_gameScene.addSystem<EnemySystem>(mb);
    m_gameScene.addSystem<MovingPlatformSystem>(mb);
    m_gameScene.addSystem<CameraTargetSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
    m_gameScene.addSystem<xy::ParticleSystem>(mb);
    m_gameScene.addSystem<xy::AudioSystem>(mb);

    m_gameScene.addDirector<NinjaDirector>(m_sprites, m_particleEmitters, m_sharedData);
    m_gameScene.addDirector<SFXDirector>(m_sharedData);

    m_uiScene.addSystem<xy::CommandSystem>(mb);
    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::SpriteAnimator>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);

    fontID = m_resources.load<sf::Font>(FontID::GearBoyFont); //bleh this should be done elsewhere for neatness
    m_uiScene.addDirector<UIDirector>(m_sharedData, m_resources.get<sf::Font>(fontID));
}

void GameState::loadResources() 
{
    m_textureIDs[TextureID::Game::Background] = m_resources.load<sf::Texture>("assets/images/"+m_sharedData.theme+"/background.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Background]).setRepeated(true);
    
    m_textureIDs[TextureID::Game::UIBackground] = m_resources.load<sf::Texture>("assets/images/"+m_sharedData.theme+"/menu_background.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::UIBackground]).setRepeated(true);

    m_textureIDs[TextureID::Game::MovingPlatform] = m_resources.load<sf::Texture>("assets/images/" + m_sharedData.theme + "/moving_platform.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::MovingPlatform]).setRepeated(true);

    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/"+m_sharedData.theme+"/player.spt", m_resources);
    //spriteSheet.loadFromFile("assets/sprites/sms/player.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Player] = spriteSheet.getSprite("player");

    m_playerAnimations[AnimID::Player::Idle] = spriteSheet.getAnimationIndex("idle", "player");
    m_playerAnimations[AnimID::Player::Jump] = spriteSheet.getAnimationIndex("jump", "player");
    m_playerAnimations[AnimID::Player::Run] = spriteSheet.getAnimationIndex("run", "player");
    m_playerAnimations[AnimID::Player::Die] = spriteSheet.getAnimationIndex("die", "player");

    spriteSheet.loadFromFile("assets/sprites/"+m_sharedData.theme+"/projectile.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Star] = spriteSheet.getSprite("projectile");

    spriteSheet.loadFromFile("assets/sprites/"+m_sharedData.theme+"/smoke_puff.spt", m_resources);
    m_sprites[SpriteID::GearBoy::SmokePuff] = spriteSheet.getSprite("smoke_puff");

    spriteSheet.loadFromFile("assets/sprites/"+m_sharedData.theme+"/checkpoint.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Checkpoint] = spriteSheet.getSprite("checkpoint");
    m_checkpointAnimations[AnimID::Checkpoint::Idle] = spriteSheet.getAnimationIndex("idle", "checkpoint");
    m_checkpointAnimations[AnimID::Checkpoint::Activate] = spriteSheet.getAnimationIndex("activate", "checkpoint");

    spriteSheet.loadFromFile("assets/sprites/"+m_sharedData.theme+"/lava.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Lava] = spriteSheet.getSprite("lava");
    auto* tex = m_sprites[SpriteID::GearBoy::Lava].getTexture();
    if (tex) //uuugghhhhhh
    {
        const_cast<sf::Texture*>(tex)->setRepeated(true);
    }

    spriteSheet.loadFromFile("assets/sprites/" + m_sharedData.theme + "/lavafall.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Lavafall] = spriteSheet.getSprite("lava");
    tex = m_sprites[SpriteID::GearBoy::Lavafall].getTexture();
    if (tex) //uuugghhhhhh
    {
        const_cast<sf::Texture*>(tex)->setRepeated(true);
    }

    spriteSheet.loadFromFile("assets/sprites/"+m_sharedData.theme+"/water.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Water] = spriteSheet.getSprite("water");
    tex = m_sprites[SpriteID::GearBoy::Water].getTexture();
    if (tex) //uuugghhhhhhghghghghghdddhhhhhh
    {
        const_cast<sf::Texture*>(tex)->setRepeated(true);
    }

    spriteSheet.loadFromFile("assets/sprites/" + m_sharedData.theme + "/waterfall.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Waterfall] = spriteSheet.getSprite("water");
    tex = m_sprites[SpriteID::GearBoy::Waterfall].getTexture();
    if (tex) //uuugghhhhhhghghghghghdddhhhhhh
    {
        const_cast<sf::Texture*>(tex)->setRepeated(true);
    }

    spriteSheet.loadFromFile("assets/sprites/"+m_sharedData.theme+"/collectibles.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Coin] = spriteSheet.getSprite("coin");
    m_sprites[SpriteID::GearBoy::Shield] = spriteSheet.getSprite("shield");
    m_sprites[SpriteID::GearBoy::Ammo] = spriteSheet.getSprite("ammo");
    m_sprites[SpriteID::GearBoy::ExtraLife] = spriteSheet.getSprite("extra_life");

    spriteSheet.loadFromFile("assets/sprites/" + m_sharedData.theme + "/shield.spt", m_resources);
    m_sprites[SpriteID::GearBoy::ShieldAvatar] = spriteSheet.getSprite("shield");

    spriteSheet.loadFromFile("assets/sprites/" + m_sharedData.theme + "/enemies.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Bird] = spriteSheet.getSprite("bird");
    m_sprites[SpriteID::GearBoy::Walker] = spriteSheet.getSprite("walker");
    m_sprites[SpriteID::GearBoy::Crawler] = spriteSheet.getSprite("crawler");
    m_sprites[SpriteID::GearBoy::Bomb] = spriteSheet.getSprite("bomb");
    m_sprites[SpriteID::GearBoy::Orb] = spriteSheet.getSprite("orb");
    m_sprites[SpriteID::GearBoy::Spitball] = spriteSheet.getSprite("spitball");

    m_crawlerAnimations[AnimID::Enemy::Walk] = spriteSheet.getAnimationIndex("walk", "crawler");
    m_crawlerAnimations[AnimID::Enemy::Die] = spriteSheet.getAnimationIndex("die", "crawler");
    m_crawlerAnimations[AnimID::Enemy::Idle] = spriteSheet.getAnimationIndex("idle", "crawler");

    spriteSheet.loadFromFile("assets/sprites/crack.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Crack] = spriteSheet.getSprite("crack");

    spriteSheet.loadFromFile("assets/sprites/" + m_sharedData.theme + "/torch.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Torch] = spriteSheet.getSprite("torch");

    spriteSheet.loadFromFile("assets/sprites/" + m_sharedData.theme + "/crate.spt", m_resources);
    m_sprites[SpriteID::GearBoy::Crate] = spriteSheet.getSprite("crate");

    m_shaders.preload(ShaderID::TileMap, tilemapFrag2, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::TileMap3D, Layer3DVertex, tilemapFrag2);
    m_shaders.preload(ShaderID::PixelTransition, PixelateFrag, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::NoiseTransition, NoiseFrag, sf::Shader::Fragment);

    m_particleEmitters[ParticleID::Shield].loadFromFile("assets/particles/" + m_sharedData.theme + "/shield.xyp", m_resources);
    m_particleEmitters[ParticleID::Checkpoint].loadFromFile("assets/particles/" + m_sharedData.theme + "/checkpoint.xyp", m_resources);
    m_particleEmitters[ParticleID::TorchSmoke].loadFromFile("assets/particles/torch_smoke.xyp", m_resources);
    m_particleEmitters[ParticleID::Crate].loadFromFile("assets/particles/" + m_sharedData.theme + "/crate.xyp", m_resources);

    m_ambience.loadFromFile("assets/sound/ambience.xas");
    m_effects.loadFromFile("assets/sound/effects.xas");
    m_gameScene.getActiveCamera().addComponent<xy::AudioEmitter>() = m_ambience.getEmitter("transition");

    //buffer texture for map
    m_tilemapBuffer.create(static_cast<std::uint32_t>(xy::DefaultSceneSize.x), static_cast<std::uint32_t>(xy::DefaultSceneSize.y), sf::ContextSettings(24));
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

        //carries the buffer to which the tilemap is drawn
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(-xy::DefaultSceneSize / 2.f);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth + 1);
        entity.addComponent<xy::Sprite>(m_tilemapBuffer.getTexture());

        m_gameScene.getActiveCamera().getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

        //map layers
        const auto& layers = m_mapLoader.getLayers();

        //for each layer create a drawable in the scene
        std::int32_t startDepth = GameConst::BackgroundDepth + 2;
        float worldDepth = -20.f;
        for (const auto& layer : layers)
        {
            entity = m_tilemapScene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Drawable>().setDepth(startDepth);
            entity.getComponent<xy::Drawable>().setTexture(layer.indexMap);
            entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::TileMap3D));
            entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_indexMap");
            entity.getComponent<xy::Drawable>().bindUniform("u_tileSet", *layer.tileSet);
            entity.getComponent<xy::Drawable>().bindUniform("u_indexSize", sf::Vector2f(layer.indexMap->getSize()));
            entity.getComponent<xy::Drawable>().bindUniform("u_tileCount", layer.tileSetSize);
            entity.getComponent<xy::Drawable>().bindUniform("u_tileSize", sf::Vector2f(m_mapLoader.getTileSize(), m_mapLoader.getTileSize()));

            entity.getComponent<xy::Drawable>().bindUniform("u_depth", worldDepth);
            //entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);

            auto& layerVerts = entity.getComponent<xy::Drawable>().getVertices();
            sf::Vector2f texCoords(layer.indexMap->getSize());
            //texCoords *= m_mapLoader.getTileSize();

            layerVerts.emplace_back(sf::Vector2f(), sf::Vector2f());
            layerVerts.emplace_back(sf::Vector2f(layer.layerSize.x, 0.f), sf::Vector2f(texCoords.x, 0.f));
            layerVerts.emplace_back(layer.layerSize, texCoords);
            layerVerts.emplace_back(sf::Vector2f(0.f, layer.layerSize.y), sf::Vector2f(0.f, texCoords.y));

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Transform>().setScale(scale, scale);

            startDepth += 4; //place layers 4 apart so props etc can be placed in between
            worldDepth += 50.f;
        }

        //create player
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(m_mapLoader.getSpawnPoint() * scale);
        entity.getComponent<xy::Transform>().setScale(scale, scale);
        entity.addComponent<xy::Drawable>().setDepth(3);
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
        entity.addComponent<xy::CommandTarget>().ID = CommandID::World::Player;
        m_playerInput.setPlayerEntity(entity);

        m_gameScene.getActiveCamera().addComponent<CameraTarget>().target = entity;
        m_gameScene.getSystem<CameraTargetSystem>().setBounds({ sf::Vector2f(), m_mapLoader.getMapSize() * scale });
        m_gameScene.getSystem<PlayerSystem>().setBounds({ sf::Vector2f(), m_mapLoader.getMapSize() * scale });

        loadCollision();
        loadEnemies();
        loadProps();
    }
    else
    {
        requestStackPush(StateID::Error);
    }
}

void GameState::loadCollision()
{
    //map collision data
    const float scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();
    const auto& collisionShapes = m_mapLoader.getCollisionShapes();
    const auto& platPaths = m_mapLoader.getPlatformPaths();

    for (const auto& shape : collisionShapes)
    {
        auto entity = m_gameScene.createEntity();
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
            case CollisionShape::MPlat:
                if (platPaths.count(shape.ID) == 0)
                {
                    //missing path, kill platform
                    m_gameScene.destroyEntity(entity);
                }
                else
                {
                    auto bounds = entity.getComponent<CollisionBody>().shapes[0].aabb;

                    //convert path to world scale
                    auto path = platPaths.at(shape.ID);
                    std::transform(path.begin(), path.end(), path.begin(),
                        [scale, bounds](sf::Vector2f v) { return (v * scale) - sf::Vector2f(bounds.width / 2.f, bounds.height / 2.f); });

                    //move plat to beginning
                    entity.getComponent<xy::Transform>().setPosition(path[0]);

                    //add path
                    entity.addComponent<MovingPlatform>().path = path;

                    //set texture
                    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::MovingPlatform]));
                    entity.getComponent<xy::Sprite>().setTextureRect(bounds);
                    entity.addComponent<xy::Drawable>();
                }
                break;
            case CollisionShape::Fluid:
            {
                sf::Vector2f size(shape.aabb.width, shape.aabb.height);
                size *= scale;

                entity.addComponent<xy::Drawable>().setDepth(1);
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
                    entity.addComponent<xy::AudioEmitter>() = m_ambience.getEmitter("lava");
                    entity.getComponent<xy::AudioEmitter>().play();
                }
                else
                {
                    spriteID = SpriteID::GearBoy::Water;
                }
                entity.getComponent<xy::Drawable>().setTexture(m_sprites[spriteID].getTexture());
                entity.addComponent<Fluid>().frameSize = m_sprites[spriteID].getTextureBounds().height;
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

                    auto bounds = m_sprites[SpriteID::GearBoy::Checkpoint].getTextureBounds();
                    bounds.width *= scale;
                    bounds.height *= scale;

                    auto particleEnt = m_gameScene.createEntity();
                    particleEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getPosition());
                    particleEnt.getComponent<xy::Transform>().move(bounds.width / 2.f, bounds.height / 3.f);
                    particleEnt.addComponent<xy::ParticleEmitter>().settings = m_particleEmitters[ParticleID::Checkpoint];

                    entity.addComponent<xy::Callback>().function =
                        [&, checkpointEnt, particleEnt](xy::Entity e, float) mutable
                    {
                        if (checkpointEnt.getComponent<xy::SpriteAnimation>().getAnimationIndex() ==
                            m_checkpointAnimations[AnimID::Checkpoint::Idle])
                        {
                            checkpointEnt.getComponent<xy::SpriteAnimation>().play(m_checkpointAnimations[AnimID::Checkpoint::Activate]);
                        }
                        particleEnt.getComponent<xy::ParticleEmitter>().start(); //do this out here so can replay effect on player spawn
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
                case GameConst::CollectibleID::Coin:
                    spriteID = SpriteID::GearBoy::Coin;
                    break;
                case GameConst::CollectibleID::Shield:
                    spriteID = SpriteID::GearBoy::Shield;
                    break;
                case GameConst::CollectibleID::Ammo:
                    spriteID = SpriteID::GearBoy::Ammo;
                    break;
                case GameConst::CollectibleID::Life:
                    spriteID = SpriteID::GearBoy::ExtraLife;
                    entity.addComponent<xy::ParticleEmitter>().settings = m_particleEmitters[ParticleID::Shield];
                    entity.getComponent<xy::ParticleEmitter>().settings.spawnOffset.x = entity.getComponent<CollisionBody>().shapes[0].aabb.width / 2.f;
                    entity.getComponent<xy::ParticleEmitter>().start();
                    break;
                }

                auto cEnt = m_gameScene.createEntity();
                cEnt.addComponent<xy::Transform>().setScale(scale, scale);
                cEnt.addComponent<xy::Drawable>();
                cEnt.addComponent<xy::Sprite>() = m_sprites[spriteID];
                cEnt.addComponent<xy::SpriteAnimation>().play(0);
                cEnt.getComponent<xy::SpriteAnimation>().setFrameID(xy::Util::Random::value(0, m_sprites[spriteID].getAnimations()[0].frameCount));
                cEnt.addComponent<BobAnimation>().parent = entity;

                entity.getComponent<xy::Transform>().addChild(cEnt.getComponent<xy::Transform>());
            }
            break;
            case CollisionShape::Dialogue:
                entity.addComponent<Dialogue>().file = m_mapLoader.getDialogueFiles()[shape.ID];
#ifdef XY_DEBUG
                entity.getComponent<Dialogue>().expired = true;
#endif
                break;
            case CollisionShape::Exit:
            {
                if (shape.ID == 1)
                {
                    auto crackEntity = m_gameScene.createEntity();
                    crackEntity.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getPosition());
                    crackEntity.getComponent<xy::Transform>().setScale(scale, scale);
                    crackEntity.addComponent<xy::Drawable>();
                    crackEntity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Crack];
                    crackEntity.addComponent<xy::SpriteAnimation>().play(0);
                    auto bounds = m_sprites[SpriteID::GearBoy::Crack].getTextureBounds();
                    crackEntity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
                    bounds = entity.getComponent<xy::BroadphaseComponent>().getArea();
                    crackEntity.getComponent<xy::Transform>().move(bounds.width / 2.f, bounds.height / 2.f);
                }
            }
            break;
            }
        }
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
        case Enemy::Crawler:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Crawler];
            entity.addComponent<Enemy>().type = Enemy::Crawler;
            entity.getComponent<Enemy>().animationIDs = m_crawlerAnimations;
            break;
        case Enemy::Bird:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Bird];
            entity.addComponent<Enemy>().type = Enemy::Bird;
            break;
        case Enemy::Walker:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Walker];
            entity.addComponent<Enemy>().type = Enemy::Walker;
            break;
        case Enemy::Orb:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Orb];
            entity.addComponent<Enemy>().type = Enemy::Orb;
            break;
        case Enemy::Spitball:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Spitball];
            entity.addComponent<Enemy>().type = Enemy::Spitball;
            entity.addComponent<xy::AudioEmitter>() = m_effects.getEmitter("fireball");
            entity.getComponent<xy::AudioEmitter>().setPitch(1.2f);
            break;
        case Enemy::Rocket:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Star];
            entity.addComponent<Enemy>().type = Enemy::Rocket;
            entity.getComponent<Enemy>().stateTime = xy::Util::Random::value(0.2f, 3.f);
            entity.getComponent<xy::Transform>().setScale(0.f, scale);
            entity.getComponent<xy::Transform>().setPosition(path.first);
            entity.addComponent<xy::AudioEmitter>() = m_effects.getEmitter("fireball");
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
        collision.shapes[0].type = (id == Enemy::Orb || id == Enemy::Spitball || id == Enemy::Rocket) ? CollisionShape::Spikes : CollisionShape::Enemy;
        collision.shapes[0].collisionFlags = CollisionShape::Player;

        entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(collision.shapes[0].type);
    }
}

void GameState::loadProps()
{
    const float scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();

    const auto& props = m_mapLoader.getProps();
    for (const auto& [id, bounds] : props)
    {
        auto entity = m_tilemapScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(bounds.left * scale, bounds.top * scale);
        entity.getComponent<xy::Transform>().setScale(scale, scale);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth + 3);

        switch (id)
        {
        default: break;
        case PropID::Torch:
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Torch];
            entity.addComponent<xy::SpriteAnimation>().play(0);
            {
                auto smokeEnt = m_gameScene.createEntity();
                smokeEnt.addComponent<xy::Transform>().setPosition(bounds.left * scale, bounds.top * scale);
                smokeEnt.getComponent<xy::Transform>().move((bounds.width / 2.f) * scale, 0.f);
                smokeEnt.addComponent<xy::ParticleEmitter>().settings = m_particleEmitters[ParticleID::TorchSmoke];
                smokeEnt.getComponent<xy::ParticleEmitter>().start();
            }
            break;
        case PropID::WaterFall:
        {
            auto audioEnt = m_gameScene.createEntity();
            audioEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getPosition());
            audioEnt.addComponent<xy::AudioEmitter>() = m_ambience.getEmitter("waterfall");
            audioEnt.getComponent<xy::AudioEmitter>().play();
        }
        //fall through
        case PropID::LavaFall:
        {
            sf::Vector2f size(bounds.width, bounds.height);

            auto& verts = entity.getComponent<xy::Drawable>().getVertices();
            verts.emplace_back(sf::Vector2f());
            verts.emplace_back(sf::Vector2f(size.x, 0.f), sf::Vector2f(size.x, 0.f));
            verts.emplace_back(size, size);
            verts.emplace_back(sf::Vector2f(0.f, size.y), sf::Vector2f(0.f, size.y));

            entity.getComponent<xy::Drawable>().updateLocalBounds();

            std::int32_t spriteID = id == PropID::LavaFall ? SpriteID::GearBoy::Lavafall : SpriteID::GearBoy::Waterfall;
            entity.getComponent<xy::Drawable>().setTexture(m_sprites[spriteID].getTexture());
            entity.addComponent<Fluid>().frameSize = m_sprites[spriteID].getTextureBounds().width;
            entity.getComponent<Fluid>().horizontal = false;

            if (m_sprites[spriteID].getAnimations()[0].framerate != 0)
            {
                entity.getComponent<Fluid>().frameTime = 1.f / m_sprites[spriteID].getAnimations()[0].framerate;
            }
        }
            break;
        case PropID::CrateSpawn:
            spawnCrate({ bounds.left * scale, bounds.top * scale });
            break;
        }
    }
}

void GameState::buildUI()
{
    auto scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();

    const float offset = 0.f;// xy::DefaultSceneSize.y - GameConst::UI::BannerHeight;

    //background
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>().setDepth(-1);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x / scale, 0.f), sf::Vector2f(xy::DefaultSceneSize.x / scale, 0.f));
    verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x / scale, GameConst::UI::BannerHeight / scale), sf::Vector2f(xy::DefaultSceneSize.x / scale, GameConst::UI::BannerHeight / scale));
    verts.emplace_back(sf::Vector2f(0.f, GameConst::UI::BannerHeight / scale), sf::Vector2f(0.f, GameConst::UI::BannerHeight / scale));
    entity.getComponent<xy::Drawable>().updateLocalBounds();
    entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::UIBackground]));

    entity.getComponent<xy::Transform>().move(0.f, offset);

    auto& font = m_resources.get<sf::Font>(fontID);

    const sf::Color* colours = GameConst::Gearboy::colours.data();
    if (m_sharedData.theme == "mes")
    {
        colours = GameConst::Mes::colours.data();
    }

    auto createText = [&](const std::string& str)->xy::Entity
    {
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>();
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Text>(font);
        entity.getComponent<xy::Text>().setCharacterSize(GameConst::UI::SmallTextSize);

        entity.getComponent<xy::Text>().setFillColour(colours[0]);
        entity.getComponent<xy::Text>().setOutlineColour(colours[2]);
        entity.getComponent<xy::Text>().setOutlineThickness(2.f);
        entity.getComponent<xy::Text>().setString(str);

        return entity;
    };


    //coin count
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(GameConst::UI::CoinPosition);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Coin];
    entity.addComponent<xy::SpriteAnimation>().play(0);
    entity.getComponent<xy::Transform>().move(-10.f, offset);

    std::stringstream ss;
    ss << "x " << std::setw(3) << std::setfill('0') << m_sharedData.inventory.coins;
    entity = createText(ss.str());
    entity.getComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, GameConst::UI::TopRow);
    entity.getComponent<xy::Transform>().move(-10.f, offset);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::CoinText;

    //ammo
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(GameConst::UI::CoinPosition.x, GameConst::UI::CoinPosition.y + 68.f);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Star];
    entity.addComponent<xy::SpriteAnimation>().play(0);

    std::stringstream se;
    se << "x " << std::setw(2) << std::setfill('0') << m_sharedData.inventory.ammo;
    entity = createText(se.str());
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, GameConst::UI::BottomRow);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::AmmoText;

    //lives
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(20.f, 6.f);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Player];
    entity.addComponent<xy::SpriteAnimation>().play(m_playerAnimations[AnimID::Player::Idle]);

    entity = createText(std::to_string(m_sharedData.inventory.lives));
    entity.getComponent<xy::Transform>().setPosition(102.f, GameConst::UI::TopRow);
    entity.getComponent<xy::Transform>().move(0.f, offset);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::LivesText;

    //score
    entity = createText("SCORE " + std::to_string(m_sharedData.inventory.score));
    entity.getComponent<xy::Transform>().setPosition(30.f, GameConst::UI::BottomRow);
    entity.getComponent<xy::Transform>().move(0.f, offset);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::ScoreText;

    //timer
    entity = createText("TIME");
    entity.getComponent<xy::Transform>().setPosition(1800.f, GameConst::UI::TopRow);
    entity.getComponent<xy::Transform>().move(0.f, offset);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);

    entity = createText(std::to_string(GameConst::RoundTime));
    entity.getComponent<xy::Transform>().setPosition(1800.f, GameConst::UI::BottomRow);
    entity.getComponent<xy::Transform>().move(0.f, offset);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::TimeText;
}

void GameState::spawnCrate(sf::Vector2f position)
{
    const float scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::GearBoy::Crate];
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);

    bounds.left -= bounds.width / 2.f;
    bounds.top -= bounds.height / 2.f;

    entity.addComponent<CollisionBody>().shapeCount = 2;
    entity.getComponent<CollisionBody>().shapes[0].aabb = bounds;
    entity.getComponent<CollisionBody>().shapes[0].type = CollisionShape::Crate;
    entity.getComponent<CollisionBody>().shapes[0].collisionFlags = CollisionGroup::CrateFlags;

    sf::FloatRect foot(bounds.left + 2.f, bounds.height / 2.f, bounds.width - 4.f, 4.f);
    entity.getComponent<CollisionBody>().shapes[1].aabb = foot;
    entity.getComponent<CollisionBody>().shapes[1].type = CollisionShape::Foot;
    entity.getComponent<CollisionBody>().shapes[1].collisionFlags = CollisionGroup::CrateFlags;

    entity.addComponent<xy::BroadphaseComponent>().setArea(xy::Util::Rectangle::combine(bounds, foot));
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionShape::Crate);

    entity.addComponent<Crate>().spawnPosition = position;
    entity.addComponent<xy::ParticleEmitter>().settings = m_particleEmitters[ParticleID::Crate];
}

void GameState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}