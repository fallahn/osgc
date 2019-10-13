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
#include "Sprite3D.hpp"
#include "HealthBarSystem.hpp"
#include "AnimationSystem.hpp"
#include "Camera3D.hpp"
#include "DepthAnimationSystem.hpp"
#include "CollisionBounds.hpp"
#include "BroadcastSystem.hpp"
#include "AnimationCallbacks.hpp"
#include "Actor.hpp"
#include "CarriableSystem.hpp"
#include "ClientCarriableCallback.hpp"
#include "WetPatchDirector.hpp"
#include "SpringFlower.hpp"
#include "ParrotSystem.hpp"
#include "BoatSystem.hpp"
#include "FlappySailSystem.hpp"
#include "BotSystem.hpp"
#include "CompassSystem.hpp"
#include "TorchlightSystem.hpp"
#include "BarrelSystem.hpp"
#include "ExplosionSystem.hpp"
#include "ClientWeaponSystem.hpp"
#include "ClientBeeSystem.hpp"
#include "ShadowCastSystem.hpp"
#include "SimpleShadowSystem.hpp"
#include "ClientDecoySystem.hpp"
#include "ClientFlareSystem.hpp"
#include "InterpolationSystem.hpp"
#include "InputParser.hpp"

#include "Operators.hpp"
#include "QuadTreeFilter.hpp"
#include "CommandIDs.hpp"
#include "GlobalConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/util/Random.hpp>
#include <xyginext/util/Math.hpp>
#include <xyginext/util/Vector.hpp>

namespace
{
    const float MaxShakeDistance = 102400.f; //closer explosions to the camera make it shake more

    std::array<sf::FloatRect, 4u> sailColours =
    {
        sf::FloatRect(80.f, 216.f, 49.f, 60.f),
        { 131.f, 216.f, 49.f, 60.f },
        { 182.f, 216.f, 49.f, 60.f },
        { 80.f, 278.f, 49.f, 60.f }
    };

    struct TimeStruct final
    {
        sf::Clock clock;
        sf::Time nextTime = sf::seconds(18.f);
    };
}

void GameState::spawnActor(Actor actor, sf::Vector2f position, std::int32_t timestamp, bool localPlayer)
{
    auto cameraEntity = m_gameScene.getActiveCamera();
    auto addHealthBar = [&](xy::Entity parent, sf::Color colour, bool killWithParent = false)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<Sprite3D>(m_modelMatrices).verticalOffset = -32.f;
        entity.getComponent<Sprite3D>().needsCorrection = false;
        entity.addComponent<HealthBar>().size = { 28.f, 4.f };
        entity.getComponent<HealthBar>().outlineThickness = 0.5f;
        entity.getComponent<HealthBar>().parent = parent;
        entity.getComponent<HealthBar>().colour = colour;
        entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderUnlit));
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::HealthBar;
        auto& healthTx = entity.addComponent<xy::Transform>();
        healthTx.setPosition(16.f, 0.f);
        healthTx.setScale(0.5f, 0.5f);
        parent.getComponent<xy::Transform>().addChild(healthTx);

        if (killWithParent)
        {
            entity.addComponent<xy::Callback>().active = true; //destroy if parent removed
            entity.getComponent<xy::Callback>().function = [&](xy::Entity ent, float)
            {
                if (ent.getComponent<xy::Transform>().getDepth() == 0)
                {
                    m_gameScene.destroyEntity(ent);
                }
            };
        }
    };
    //auto spawnDog = [&](std::size_t playerNumber)
    //{
    //    static const std::array<sf::Vector2f, 4u> dogPos =
    //    {
    //        sf::Vector2f(Global::TileSize, Global::TileSize / 2.f),
    //        sf::Vector2f(-Global::TileSize, Global::TileSize / 2.f),
    //        sf::Vector2f(-Global::TileSize, -Global::TileSize / 2.f),
    //        sf::Vector2f(Global::TileSize, -Global::TileSize / 2.f)
    //    };
    //
    //    auto entity = m_gameScene.createEntity();
    //    //entity.addComponent<Dog>().owner = playerNumber;
    //    auto& spriteTx = entity.addComponent<xy::Transform>();
    //    entity.addComponent<xy::Drawable>();
    //    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Dog];
    //    entity.addComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::Dog][AnimationID::IdleDown]);
    //    entity.addComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Dog];
    //    entity.getComponent<AnimationModifier>().nextAnimation = m_animationMaps[SpriteID::Dog][AnimationID::IdleDown];
    //    entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
    //    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Dog].getTexture());
    //    entity.addComponent<Sprite3D>(m_modelMatrices);
    //    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    //    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    //    entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("dog");
    //
    //    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    //    spriteTx.setOrigin(bounds.width / 2.f, 0.f);
    //    spriteTx.setPosition(position + dogPos[playerNumber - Actor::ID::PlayerOne]); //THIS WILL BREAK IF THE ENUM CHANGES!!!!!
    //    spriteTx.setScale(0.5f, -0.5f);
    //
    //    if (playerNumber == Actor::PlayerOne || playerNumber == Actor::PlayerFour) spriteTx.scale(-1.f, 1.f);
    //
    //    //shadow
    //    entity = m_gameScene.createEntity();
    //    auto& shadowTx = entity.addComponent<xy::Transform>();
    //    shadowTx.setOrigin(8.f, 8.f);
    //    shadowTx.setPosition(bounds.width / 2.f, 0.f);
    //    entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::PlaneShader));
    //    entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/player_shadow.png"));
    //    spriteTx.addChild(shadowTx);
    //};
    auto addNameTag = [&](xy::Entity parent)
    {
        auto idx = parent.getComponent<Actor>().id - Actor::ID::PlayerOne;

        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Sprite>(m_nameTagManager.getTexture(idx));
        entity.addComponent<xy::Drawable>();
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderUnlitTextured));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", m_nameTagManager.getTexture(idx));
        entity.addComponent<Sprite3D>(m_modelMatrices).verticalOffset = -34.f;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        parent.getComponent<xy::Transform>().addChild(entity.addComponent<xy::Transform>());
        entity.getComponent<xy::Transform>().setScale(0.15f, 0.15f); //text is rendered very large then scaled down to prevent aliasing
        entity.getComponent<xy::Transform>().move(6.f, 0.f);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::NameTag;
    };
    auto addRings = [&](xy::Entity parentEnt)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::PlaneShader));
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Rings];
        entity.addComponent<xy::SpriteAnimation>().play(0);
        auto& ringTx = entity.addComponent<xy::Transform>();
        auto bounds = m_sprites[SpriteID::Rings].getTextureBounds();
        ringTx.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        ringTx.setPosition(parentEnt.getComponent<xy::Sprite>().getTextureBounds().width / 2.f, 0.f);
        ringTx.setScale(0.f, 0.f);
        parentEnt.getComponent<xy::Transform>().addChild(ringTx);
        parentEnt.addComponent<DepthComponent>().ringEffect = entity;
    };
    auto addShadow = [&](xy::Entity entity)
    {
        auto ent = m_gameScene.createEntity();
        ent.addComponent<xy::Transform>();
        ent.addComponent<xy::Drawable>();
        ent.addComponent<ShadowCaster>().parent = entity;
    };
    auto addSpawnPuff = [&](xy::Entity entity)
    {
        auto puffEnt = createPlayerPuff({}, true);
        puffEnt.getComponent<xy::Transform>().move(0.f, -4.f);
        entity.getComponent<xy::Transform>().addChild(puffEnt.getComponent<xy::Transform>());
        entity.addComponent<std::any>() = std::make_any<xy::Entity>(puffEnt);
    };

    auto addDebugLabel = [&](xy::Entity parent)
    {
#ifdef XY_DEBUG
        auto& font = m_fontResource.get();
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setScale(0.25f, -0.25f);
        entity.addComponent<xy::Text>(font).setString("DEBUG");
        entity.getComponent<xy::Text>().setCharacterSize(16);
        entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
        entity.addComponent<Sprite3D>(m_modelMatrices).verticalOffset = -32.f;
        entity.getComponent<Sprite3D>().needsCorrection = false;
        entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderUnlitTextured));
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", font.getTexture(entity.getComponent<xy::Text>().getCharacterSize()));
        entity.addComponent<std::uint32_t>() = parent.getComponent<Actor>().serverID;
        entity.addComponent<xy::CommandTarget>().ID = CommandID::DebugLabel;

        entity.addComponent<xy::Callback>().active = true; //destroy if parent removed
        entity.getComponent<xy::Callback>().function = [&](xy::Entity ent, float)
        {
            if (ent.getComponent<xy::Transform>().getDepth() == 0)
            {
                m_gameScene.destroyEntity(ent);
            }
        };

        parent.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
#endif
    };

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::SpriteAnimation>();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.addComponent<Actor>() = actor;
    entity.getComponent<Actor>().entityID = entity.getIndex();
    entity.addComponent<AnimationModifier>();
    entity.addComponent<xy::Drawable>();

    InterpolationPoint initialPoint;
    initialPoint.position = position;
    initialPoint.timestamp = timestamp;

    bool hasWeapon = false;
    switch (actor.id)
    {
    default:break;
    case Actor::PlayerOne:
        //sprite for player
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::PlayerOne];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::PlayerOne];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::PlayerOne][AnimationID::IdleDown]);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::PlayerOne].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::PlayerBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Player | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Player;
        entity.getComponent<CollisionComponent>().collidesBoat = true;
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("walk");
        entity.addComponent<BroadcastComponent>();
        hasWeapon = true;
        addNameTag(entity);
        addShadow(entity);
        addSpawnPuff(entity);
        break;

    case Actor::PlayerTwo:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::PlayerTwo];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::PlayerTwo];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::PlayerTwo][AnimationID::IdleDown]);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::PlayerTwo].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::PlayerBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Player | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Player;
        entity.getComponent<CollisionComponent>().collidesBoat = true;
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("walk");
        entity.addComponent<BroadcastComponent>();
        hasWeapon = true;
        addNameTag(entity);
        addShadow(entity);
        addSpawnPuff(entity);
        break;
    case Actor::PlayerThree:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::PlayerThree];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::PlayerThree];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::PlayerThree][AnimationID::IdleDown]);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::PlayerThree].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::PlayerBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Player | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Player;
        entity.getComponent<CollisionComponent>().collidesBoat = true;
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("walk");
        entity.addComponent<BroadcastComponent>();
        hasWeapon = true;
        addNameTag(entity);
        addShadow(entity);
        addSpawnPuff(entity);
        break;
    case Actor::PlayerFour:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::PlayerFour];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::PlayerFour];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::PlayerFour][AnimationID::IdleDown]);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::PlayerFour].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::PlayerBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Player | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Player;
        entity.getComponent<CollisionComponent>().collidesBoat = true;
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("walk");
        entity.addComponent<BroadcastComponent>();
        hasWeapon = true;
        addNameTag(entity);
        addShadow(entity);
        addSpawnPuff(entity);
        break;
    case Actor::ID::Bees:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Bees];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Bees];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::IdleDown;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::Bees][AnimationID::IdleDown]);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Bees].getTexture());
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("bees");
        entity.getComponent<xy::AudioEmitter>().play();
        entity.addComponent<ClientBees>().defaultVolume = entity.getComponent<xy::AudioEmitter>().getVolume();
        entity.getComponent<ClientBees>().homePosition = position;

        {
            //beehive
            auto hiveEnt = m_gameScene.createEntity();
            hiveEnt.addComponent<xy::Transform>().setPosition(position);
            hiveEnt.getComponent<xy::Transform>().move(0.f, -1.f);
            hiveEnt.getComponent<xy::Transform>().setScale(0.5f, -0.5f);
            hiveEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::Beehive];
            auto offset = m_sprites[SpriteID::Beehive].getTextureBounds().width * 0.6f;
            hiveEnt.getComponent<xy::Transform>().setOrigin(offset, 0.f);

            hiveEnt.addComponent<Sprite3D>(m_modelMatrices);
            hiveEnt.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
            hiveEnt.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Bees].getTexture());
            hiveEnt.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
            hiveEnt.getComponent<xy::Drawable>().bindUniform("u_modelMat", &hiveEnt.getComponent<Sprite3D>().getMatrix()[0][0]);
        }
        break;
    case Actor::ID::Barrel:
    {
        auto idx = SpriteID::BarrelOne + xy::Util::Random::value(0, 1);
        entity.addComponent<xy::Sprite>() = m_sprites[idx];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[idx];
        entity.getComponent<AnimationModifier>().nextAnimation = 0;
        entity.getComponent<xy::SpriteAnimation>().play(0);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Sprite>().getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds * 2.f);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Barrel | QuadTreeFilter::BotQuery);
        entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        entity.getComponent<CollisionComponent>().ID = ManifoldID::Barrel;
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("barrel");
        entity.addComponent<xy::Callback>().function = BarrelAudioCallback();
        entity.getComponent<xy::Callback>().active = true;
        addRings(entity);
        addShadow(entity);
    }
    break;
    case Actor::Decoy:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Decoy];
        entity.addComponent<ClientDecoy>();
        entity.getComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::Decoy].getTexture());
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Decoy].getTexture());
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Decoy];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Spawn;
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("decoy");
        entity.getComponent<xy::AudioEmitter>().play();
        addShadow(entity);
        break;
    case Actor::SkullShield:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::SkullShield];
        entity.addComponent<ClientDecoy>(); //use the decoy animation cos it looks cool.
        entity.getComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::SkullShield].getTexture());
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::SkullShield].getTexture());
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::SkullShield];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Idle;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::SkullShield][AnimationID::Idle]);
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("shield");
        entity.getComponent<xy::AudioEmitter>().play();
        addShadow(entity);
        break;
    case Actor::Flare:
        //smoke puff
    {
        auto smokeEnt = m_gameScene.createEntity();
        smokeEnt.addComponent<xy::Transform>().setPosition(position);
        smokeEnt.addComponent<Actor>().id = Actor::SmokePuff;
        smokeEnt.getComponent<Actor>().entityID = smokeEnt.getIndex();
        smokeEnt.getComponent<Actor>().serverID = actor.serverID; //destroyed when server kills flare.
        smokeEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::SmokePuff];
        smokeEnt.addComponent<Sprite3D>(m_modelMatrices).verticalOffset = -5.f;
        smokeEnt.addComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::SmokePuff].getTexture());
        smokeEnt.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        smokeEnt.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::SmokePuff].getTexture());
        smokeEnt.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        smokeEnt.getComponent<xy::Drawable>().bindUniform("u_modelMat", &smokeEnt.getComponent<Sprite3D>().getMatrix()[0][0]);
        smokeEnt.getComponent<xy::Transform>().setScale(1.f, -1.f);
        smokeEnt.getComponent<xy::Transform>().move(0.f, 2.f);
        smokeEnt.getComponent<xy::Transform>().setOrigin(8.f, 0.f);
        smokeEnt.addComponent<xy::SpriteAnimation>().play(0);
        smokeEnt.addComponent<xy::CommandTarget>().ID = CommandID::Removable;

        entity.addComponent<ClientFlare>();
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Removable;
        entity.addComponent<Sprite3D>(m_modelMatrices).needsCorrection = false;
        entity.getComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::Flare].getTexture());
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Flare].getTexture());
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Transform>().setScale(0.5f, 0.5f);

        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("flare");
        entity.getComponent<xy::AudioEmitter>().play();

        //raise a message so sky system can be pinked
        auto* msg = getContext().appInstance.getMessageBus().post<MapEvent>(MessageID::MapMessage);
        msg->type = MapEvent::FlareLaunched;
    }
        //bail and skip shadow
        return;
    case Actor::DirtSpray:
        entity.getComponent<xy::Drawable>().setPrimitiveType(sf::Quads);

        entity.addComponent<Sprite3D>(m_modelMatrices).needsCorrection = false;
        entity.getComponent<xy::Drawable>().setTexture(&m_textureResource.get("assets/images/dirt.png"));
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Drawable>().getTexture());
        entity.addComponent<Explosion>().type = Explosion::Dirt;
        entity.addComponent<xy::CommandTarget>().ID = CommandID::NetInterpolator;
        entity.addComponent<InterpolationComponent>(initialPoint).setEnabled(false);
        return;

    case Actor::ID::Lightning:
    {
        //we create a specific lightning sprite then
        //fall through to the explosion case below
        //having the same serverID as the explosion means both ents are removed
        //at the same time.
        auto lightningEnt = m_gameScene.createEntity();
        lightningEnt.addComponent<xy::SpriteAnimation>();
        lightningEnt.addComponent<xy::Transform>().setPosition(position);
        lightningEnt.getComponent<xy::Transform>().scale(0.25f, -1.f);
        lightningEnt.addComponent<Actor>() = actor;
        lightningEnt.getComponent<Actor>().entityID = lightningEnt.getIndex();
        lightningEnt.addComponent<AnimationModifier>();
        lightningEnt.addComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);

        lightningEnt.addComponent<xy::Sprite>(m_textureResource.get("assets/images/lightning.png"));
        lightningEnt.addComponent<Sprite3D>(m_modelMatrices);
        lightningEnt.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderUnlitTextured));
        lightningEnt.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        lightningEnt.getComponent<xy::Drawable>().bindUniform("u_modelMat", &lightningEnt.getComponent<Sprite3D>().getMatrix()[0][0]);
        lightningEnt.getComponent<xy::Drawable>().bindUniform("u_texture", *lightningEnt.getComponent<xy::Sprite>().getTexture());
        lightningEnt.addComponent<xy::CommandTarget>().ID = CommandID::NetInterpolator;
        lightningEnt.addComponent<InterpolationComponent>(initialPoint).setEnabled(false);
        lightningEnt.addComponent<xy::Callback>().function = FadeoutCallback();
        lightningEnt.getComponent<xy::Callback>().active = true;

        auto* msg = getContext().appInstance.getMessageBus().post<MapEvent>(MessageID::MapMessage);
        msg->type = MapEvent::LightningStrike;
        msg->position = position;
        msg->value = 0.01f;
    }

    case Actor::Explosion:
    {//if we're falling through from the case above make sure this ent has correct actor ID
        bool raiseMessage = true;
        if (entity.getComponent<Actor>().id != Actor::Explosion)
        {
            entity.getComponent<Actor>().id = Actor::ID::Explosion;
            raiseMessage = false; //ignore for lightning
        }
        entity.getComponent<xy::Drawable>().setPrimitiveType(sf::Quads);
        entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);

        entity.addComponent<xy::CommandTarget>().ID = CommandID::NetInterpolator;
        entity.addComponent<InterpolationComponent>(initialPoint).setEnabled(false);

        bool inWater = m_islandGenerator.isWater(position);
        if (!inWater)
        {
            entity.addComponent<Sprite3D>(m_modelMatrices).needsCorrection = false;
            entity.getComponent<xy::Drawable>().setTexture(&m_textureResource.get("assets/images/explosion.png"));
            entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
            entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
            entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
            entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Drawable>().getTexture());

            entity.addComponent<Explosion>();
        }

        //add some camera shake if nearby
        {
            auto camPos = cameraEntity.getComponent<xy::Transform>().getWorldPosition();
            float amount = xy::Util::Math::clamp(xy::Util::Vector::lengthSquared(camPos - position) / MaxShakeDistance, 0.f, 1.f);
            auto& cam = cameraEntity.getComponent<Camera3D>();
            cam.shakeAmount = std::max(1.f - amount, cam.shakeAmount);

            if (raiseMessage)
            {
                auto* msg = getContext().appInstance.getMessageBus().post<MapEvent>(MessageID::MapMessage);
                msg->type = MapEvent::Explosion;
                msg->position = position;
                
                //impact sprite
                auto impactEnt = m_gameScene.createEntity();
                impactEnt.addComponent<xy::Transform>().setPosition(position);
                impactEnt.addComponent<Actor>().id = Actor::Impact;
                impactEnt.getComponent<Actor>().entityID = impactEnt.getIndex();
                impactEnt.getComponent<Actor>().serverID = actor.serverID; //destroyed when server kills explosion.
                impactEnt.addComponent<xy::CommandTarget>().ID = CommandID::Removable;
                impactEnt.addComponent<xy::Sprite>() = m_sprites[SpriteID::Impact];
                impactEnt.addComponent<Sprite3D>(m_modelMatrices);
                impactEnt.addComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::Impact].getTexture());
                impactEnt.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
                impactEnt.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Impact].getTexture());
                impactEnt.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
                impactEnt.getComponent<xy::Drawable>().bindUniform("u_modelMat", &impactEnt.getComponent<Sprite3D>().getMatrix()[0][0]);
                impactEnt.getComponent<xy::Transform>().setScale(0.5f, -0.5f);
                impactEnt.getComponent<xy::Transform>().move(0.f, -2.f);
                impactEnt.getComponent<xy::Transform>().setOrigin(m_sprites[SpriteID::Impact].getTextureBounds().width / 2.f, 0.f);
                if (inWater)
                {
                    impactEnt.addComponent<xy::SpriteAnimation>().play(1);
                    impactEnt.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("splash");
                    impactEnt.getComponent<xy::AudioEmitter>().play();
                }
                else
                {
                    impactEnt.addComponent<xy::SpriteAnimation>().play(0);
                }
            }
        }
    }
        return;

    case Actor::Crab:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Crab];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Crab];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::WalkLeft;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::Crab][AnimationID::WalkLeft]);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Crab].getTexture());

        entity.addComponent<Sprite3D>(m_modelMatrices);
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);
        entity.addComponent<InterpolationComponent>(initialPoint);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::NetInterpolator;
        {
            auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
            entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);
        }
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("crab");
        entity.getComponent<xy::AudioEmitter>().play();
        addShadow(entity);
        return; //skip shadows
    case Actor::Seagull:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Seagull];
        entity.getComponent<xy::SpriteAnimation>().play(0);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");

        entity.addComponent<Sprite3D>(m_modelMatrices);
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);
        {
            auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
            entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);
        }
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("seagull");
        entity.addComponent<float>() = entity.getComponent<xy::AudioEmitter>().getVolume(); //store the default volume to be restored during day time
        entity.addComponent<xy::CommandTarget>().ID = CommandID::DaySound;
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().userData = std::make_any<TimeStruct>();
        entity.getComponent<xy::Callback>().function =
            [](xy::Entity e, float)
        {
            auto& timer = std::any_cast<TimeStruct&>(e.getComponent<xy::Callback>().userData);
            if (timer.clock.getElapsedTime() > timer.nextTime)
            {
                timer.nextTime = sf::seconds(xy::Util::Random::value(18, 35));
                timer.clock.restart();
                e.getComponent<xy::AudioEmitter>().play();
                e.getComponent<xy::SpriteAnimation>().play(1);
            }

            if (e.getComponent<xy::AudioEmitter>().getStatus() == xy::AudioEmitter::Stopped)
            {
                e.getComponent<xy::SpriteAnimation>().play(0);
            }
        };
        
        return; //skip shadows
    case Actor::WaterDetail:
    {
        static int lastSprite = 0;
        int spr = lastSprite;
        do
        {
            //TODO make sure no inf loop here!
            spr = xy::Util::Random::value(SpriteID::Posts, SpriteID::SmallRock02);
        } while (spr == lastSprite);
        
        lastSprite = spr;
        entity.addComponent<xy::Sprite>() = m_sprites[spr];
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");

        entity.addComponent<Sprite3D>(m_modelMatrices);
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        float xScale = (xy::Util::Random::value(0, 1) == 0) ? 0.5f : -0.5f;
        entity.getComponent<xy::Transform>().setScale(xScale, -0.5f);
        
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);
    }
        return; //skip shadows
    case Actor::Lantern:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Lantern];
        entity.getComponent<xy::SpriteAnimation>().play(0);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Torchlight>();
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Torch;
        entity.getComponent<Carriable>().spawnPosition = position; //there's no clientside system to set this
        entity.addComponent<xy::Callback>().function = ClientCarriableCallback(position);

        m_gameScene.getSystem<ShadowCastSystem>().addLight(entity);
        m_gameScene.getSystem<SimpleShadowSystem>().addLight(entity);
        break;

    case Actor::Treasure:
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Treasure];
        entity.getComponent<AnimationModifier>().nextAnimation = m_animationMaps[SpriteID::Treasure][AnimationID::IdleDown];
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Treasure];
        entity.getComponent<xy::SpriteAnimation>().play(3);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Treasure].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Treasure; //used to sync carry state with server
        entity.getComponent<Carriable>().spawnPosition = position;
        entity.addComponent<xy::Callback>().function = ClientCarriableCallback(position);
        addShadow(entity);
        break;
    case Actor::DecoyItem:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::DecoyItem];
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::DecoyItem].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Decoy; //used to sync carry state with server
        entity.getComponent<Carriable>().spawnPosition = position;
        entity.addComponent<xy::Callback>().function = ClientCarriableCallback(position);
        addShadow(entity);
        break;
    case Actor::MineItem:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::MineItem];
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::MineItem].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Mine; //used to sync carry state with server
        entity.getComponent<Carriable>().spawnPosition = position;
        entity.addComponent<xy::Callback>().function = ClientCarriableCallback(position);
        addShadow(entity);
        break;
    case Actor::SkullItem:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::SkullItem];
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::SkullItem].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::SpookySkull; //used to sync carry state with server
        entity.getComponent<Carriable>().spawnPosition = position;
        entity.addComponent<xy::Callback>().function = ClientCarriableCallback(position);
        addShadow(entity);
        break;
    case Actor::FlareItem:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::FlareItem];
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::FlareItem].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::LanternBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        entity.addComponent<Carriable>().type = Carrier::Flare; //used to sync carry state with server
        entity.getComponent<Carriable>().spawnPosition = position;
        entity.getComponent<Carriable>().offsetMultiplier = Carriable::FlareOffset;
        entity.addComponent<xy::Callback>().function = ClientCarriableCallback(position);
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::FlareItem];
        addShadow(entity);
        break;
    case Actor::Parrot:
        entity.addComponent<Sprite3D>(m_modelMatrices).needsCorrection = false;
        entity.getComponent<xy::Drawable>().setTexture(&m_textureResource.get("assets/images/parrot.png"));
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Drawable>().getTexture());
        entity.addComponent<ParrotFlock>();
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Parrot;
        entity.addComponent<xy::AudioEmitter>().setSource(m_audioResource.get("assets/sound/effects/bird_flight.wav"));
        entity.getComponent<xy::AudioEmitter>().setAttenuation(1.f);
        entity.getComponent<xy::AudioEmitter>().setMinDistance(20.f);
        entity.getComponent<xy::AudioEmitter>().setVolume(9.f);
        return; //skip shadows
    case Actor::TreasureSpawn:
    case Actor::AmmoSpawn:
    case Actor::CoinSpawn:
        m_miniMap.addCross(position);
        m_gameScene.getSystem<BotSystem>().addDiggableSpot(position);
    case Actor::MineSpawn: //don't draw these on the minimap
        entity.getComponent<AnimationModifier>().animationMap = { 0, 1, 2, 3 };
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::WetPatch];
        entity.addComponent<xy::BroadphaseComponent>().setArea({ 0.f, 0.f, Global::TileSize, Global::TileSize });
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::WetPatch);
        entity.addComponent<WetPatch>(); //required to sync with server so client bots know to dig here
        entity.getComponent<xy::SpriteAnimation>().play(0);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::LandShader));
        entity.getComponent<xy::Drawable>().setDepth(Global::MaxSortingDepth - 2);
        entity.getComponent<xy::Drawable>().bindUniform("u_diffuseTexture", *m_sprites[SpriteID::WetPatch].getTexture());
        entity.getComponent<xy::Drawable>().bindUniform("u_normalTexture", *m_sprites[SpriteID::WetPatch].getTexture());
        entity.getComponent<xy::Drawable>().bindUniform("u_normalUVOffset", -0.5f);

        //although static we still need to receive animation updates
        entity.addComponent<InterpolationComponent>(initialPoint);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::NetInterpolator;

        //scatter some grass
        if (actor.id != Actor::MineSpawn)
        {
            static const float offsetPos = 24.f + (Global::TileSize / 2.f);
            static size_t posIdx = 0;
            static const std::array<sf::Vector2f, 8u> grassOffsets =
            {
                sf::Vector2f(0.f, offsetPos - 2.f),
                sf::Vector2f(offsetPos, offsetPos - 2.f),
                sf::Vector2f(offsetPos, 0.f),
                sf::Vector2f(offsetPos, -offsetPos - 2.f),
                sf::Vector2f(0.f, -offsetPos - 2.f),
                sf::Vector2f(-offsetPos, -offsetPos - 2.f),
                sf::Vector2f(-offsetPos, 0.f),
                sf::Vector2f(-offsetPos, offsetPos - 2.f)
            };

            static const std::array<float, 3u> textureOffset = { 0.f, 16.f, 32.f };

            for (auto i = 0; i < 3; ++i)
            {
                auto grassEnt = m_gameScene.createEntity();
                grassEnt.addComponent<Sprite3D>(m_modelMatrices).verticalOffset = 8.f;
                grassEnt.addComponent<SpringFlower>(-16.f);
                grassEnt.getComponent<SpringFlower>().textureRect = { 0.f, textureOffset[i], 48.f, 16.f };
                grassEnt.getComponent<SpringFlower>().mass = 0.14f;
                grassEnt.getComponent<SpringFlower>().stiffness = -14.f;
                grassEnt.getComponent<SpringFlower>().damping = -0.1f;
                grassEnt.addComponent<xy::Drawable>();// .setPrimitiveType(sf::Quads);
                grassEnt.getComponent<xy::Drawable>().setTexture(&m_textureResource.get("assets/images/grass.png"));
                grassEnt.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
                grassEnt.getComponent<xy::Drawable>().bindUniform("u_texture", *grassEnt.getComponent<xy::Drawable>().getTexture());
                grassEnt.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
                grassEnt.getComponent<xy::Drawable>().bindUniform("u_modelMat", &grassEnt.getComponent<Sprite3D>().getMatrix()[0][0]);
                grassEnt.addComponent<xy::Transform>().setPosition((position + sf::Vector2f(Global::TileSize / 2.f, Global::TileSize / 2.f)) + grassOffsets[posIdx]);
                grassEnt.getComponent<xy::Transform>().move(xy::Util::Random::value(-24.f, -22.f), 0.f);
                grassEnt.getComponent<xy::Transform>().setScale(0.5f, -0.5f);

                posIdx = (posIdx + xy::Util::Random::value(1, 3)) % grassOffsets.size();
            }
        }
        else
        {
            //in case mine is planted on top of existing treasure
            entity.getComponent<xy::Drawable>().setDepth(Global::MaxSortingDepth - 1);
            //this is a fudge to stop client bots digging their own traps - doesn't
            //affect when playing with human players
            entity.getComponent<WetPatch>().owner = Actor::PlayerOne;
        }
        {
            //raise a message because we return from here
            auto* msg = getContext().appInstance.getMessageBus().post<ActorEvent>(MessageID::ActorMessage);
            msg->type = ActorEvent::Spawned;
            msg->position = position;
            msg->id = actor.id;
        }

        return; //static sprites don't need shadows
    case Actor::Ammo:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Ammo];
        entity.addComponent<xy::Callback>().function = CollectibleAnimationCallback();
        entity.getComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Ammo].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::CollectibleBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        break;
    case Actor::Food:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Food];
        entity.addComponent<xy::Callback>().function = CollectibleAnimationCallback();
        entity.getComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Food].getTexture());
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::CollectibleBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        break;
    case Actor::Coin:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Coin];
        entity.addComponent<xy::Callback>().function = CollectibleAnimationCallback();
        entity.getComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Coin].getTexture());
        entity.getComponent<xy::SpriteAnimation>().play(0);
        {
            sf::FloatRect bounds((-Global::TileSize / 2.f), (-Global::TileSize / 2.f), Global::TileSize, Global::TileSize);
            entity.addComponent<xy::BroadphaseComponent>().setArea(bounds);
            entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
        }
        break;
    case Actor::Fire:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Fire];
        entity.addComponent<Sprite3D>(m_modelMatrices).verticalOffset = 32.f;

        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Fire].getTexture());
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::SpriteAnimation>().play(0);
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Fire];
        entity.getComponent<AnimationModifier>().nextAnimation = m_animationMaps[SpriteID::Fire][AnimationID::Spawn];
        entity.getComponent<xy::Transform>().setPosition(0.f, -2.f);
        entity.addComponent<xy::Callback>().function = FlameAnimationCallback(m_gameScene);

        {
            auto targetID = actor.serverID;
            entity.getComponent<Actor>().serverID = -1; //I know it's not unsigned but don't want to accidentally clone another server ID
            xy::Command cmd;
            cmd.targetFlags = CommandID::InventoryHolder;
            cmd.action = [&, entity, targetID](xy::Entity e, float) mutable
            {
                if(e.getComponent<Actor>().serverID == targetID)
                {
                    e.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
                    entity.getComponent<xy::Callback>().active = true;
                }
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        return;
    case Actor::Skeleton:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Skeleton];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Skeleton];
        entity.getComponent<AnimationModifier>().nextAnimation = AnimationID::Spawn;
        entity.getComponent<xy::SpriteAnimation>().play(m_animationMaps[SpriteID::Skeleton][AnimationID::Spawn]);
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Skeleton].getTexture());
        entity.addComponent<xy::Callback>().function = RedFlashCallback();
        //entity.addComponent<CollisionComponent>().bounds = Global::PlayerBounds;
        //entity.getComponent<CollisionComponent>().ID = ManifoldID::Skeleton; 
        entity.addComponent<xy::BroadphaseComponent>().setArea(Global::PlayerBounds);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery); //used by bot detection
        entity.addComponent<xy::AudioEmitter>() = m_audioScape.getEmitter("walk");
        entity.addComponent<Inventory>();
        addHealthBar(entity, sf::Color::Red, true);
        addShadow(entity);

        addDebugLabel(entity);

        break;
    case Actor::Boat:
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Boat];
        entity.getComponent<AnimationModifier>().animationMap = m_animationMaps[SpriteID::Boat];
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Boat].getTexture());

        auto halfSize = Global::IslandSize / 2.f;
        std::size_t playerNumber = 0;
        //Actor::ID id = Actor::ID::PlayerOne;
        if (position.x > halfSize.x && position.y < halfSize.y)
        {
            playerNumber = 1;
            //id = Actor::PlayerTwo;
        }
        else if (position.x > halfSize.x && position.y > halfSize.y)
        {
            playerNumber = 2;
            //id = Actor::PlayerThree;
        }
        else if (position.x < halfSize.x && position.y > halfSize.y)
        {
            playerNumber = 3;
            //id = Actor::PlayerFour;
        }

        //spawnDog(id);

        {
            auto collEnt = m_gameScene.createEntity();
            collEnt.addComponent<xy::Transform>().setPosition(position);
            collEnt.getComponent<xy::Transform>().move(-Global::BoatBounds.width / 4.f, 0.f);
            collEnt.addComponent<CollisionComponent>().bounds = Global::BoatBounds;
            collEnt.getComponent<CollisionComponent>().collidesTerrain = false;
            collEnt.getComponent<CollisionComponent>().ID = ManifoldID::Boat;
            collEnt.addComponent<xy::BroadphaseComponent>().setArea(Global::BoatBounds);
            collEnt.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::Boat | QuadTreeFilter::BotQuery);
            collEnt.addComponent<Boat>().playerNumber = playerNumber;
            collEnt.addComponent<Actor>() = actor;

            auto sailEnt = m_gameScene.createEntity();
            sailEnt.addComponent<xy::Transform>().setPosition(50.f, 0.01f);
            sailEnt.getComponent<xy::Transform>().setScale(0.5f, 0.5f);
            sailEnt.addComponent<xy::Drawable>().setTexture(m_sprites[SpriteID::Boat].getTexture());
            sailEnt.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
            sailEnt.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Boat].getTexture());

            sailEnt.addComponent<FlappySail>().clothRect = sailColours[playerNumber];
            sailEnt.getComponent<FlappySail>().poleRect = { 0.f, 216.f, 74.f, 64.f };

            sailEnt.addComponent<Sprite3D>(m_modelMatrices).needsCorrection = false;
            sailEnt.getComponent<Sprite3D>().verticalOffset = -10.2f;
            sailEnt.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
            sailEnt.getComponent<xy::Drawable>().bindUniform("u_modelMat", &sailEnt.getComponent<Sprite3D>().getMatrix()[0][0]);

            entity.getComponent<xy::Transform>().addChild(sailEnt.getComponent<xy::Transform>());
        }
        addShadow(entity);
        break;
    }

    if (localPlayer)
    {
        entity.addComponent<InputComponent>();
        entity.addComponent<Player>().spawnPosition = position;
        entity.addComponent<Carrier>();
        //need to set player number according to server so correct player index
        //is sent when sending input.
        switch (actor.id)
        {
        default:
        case Actor::PlayerOne:
            break;
        case Actor::PlayerTwo:
            entity.getComponent<Player>().playerNumber = 1;
            break;
        case Actor::PlayerThree:
            entity.getComponent<Player>().playerNumber = 2;
            break;
        case Actor::PlayerFour:
            entity.getComponent<Player>().playerNumber = 3;
            break;
        }

        entity.addComponent<xy::CommandTarget>().ID = CommandID::LocalPlayer;
        cameraEntity.getComponent<Camera3D>().target = entity;

        m_inputParser.setPlayerEntity(entity, entity.getComponent<Player>().playerNumber);
        entity.addComponent<Bot>();// .enabled = true; //this component is required by the input parser

        auto compassEntity = m_gameScene.createEntity();
        compassEntity.addComponent<xy::Drawable>();
        compassEntity.getComponent<xy::Drawable>().setTexture(&m_textureResource.get("assets/images/compass.png"));
        compassEntity.addComponent<xy::Transform>();
        compassEntity.addComponent<Compass>().parent = entity;

        //spawnCurseIcon(entity, 0);

#ifdef XY_DEBUG
        auto debugEnt = m_gameScene.createEntity();
        debugEnt.addComponent<xy::Callback>().active = true;
        debugEnt.getComponent<xy::Callback>().function = [&, entity](xy::Entity, float)
        {
            const auto& bot = entity.getComponent<Bot>();

            static const std::array<std::string, 5> stateStrings =
            {
                "Searching", "Digging", "Fighting", "Targeting", "Capturing"
            };
            static const std::array<std::string, 7> targetStrings =
            {
                " None", " Hole", " Treasure", " Light", " Collectible", " Enemy", " Item"
            };

            std::string strState = stateStrings[(int)bot.state];
            //if (bot.state == Bot::State::Targeting)
            {
                strState += targetStrings[(int)bot.targetType] + " at " + std::to_string(bot.targetPoint.x) + ", " + std::to_string(bot.targetPoint.y);
            }

            xy::App::printStat("Bot State", strState);
            xy::App::printStat("Fleeing", bot.fleeing ? "true" : "false");

            auto position = entity.getComponent<xy::Transform>().getPosition();
            xy::App::printStat("Position", std::to_string(position.x) + ", " + std::to_string(position.y));
            xy::App::printStat("Path Size", std::to_string(bot.path.size()));
        };
#endif //XY_DEBUG
    }
    else
    {
        //this is a net client
        entity.addComponent<InterpolationComponent>(initialPoint);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::NetInterpolator;

        if (actor.id == Actor::ID::Skeleton)
        {
            entity.getComponent<xy::CommandTarget>().ID |= CommandID::InventoryHolder;
        }
    }

    //set 3D sprite properties
    entity.addComponent<Sprite3D>(m_modelMatrices);
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);

    //update origin and shadow position based on sprite
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);

    auto& spriteTx = entity.getComponent<xy::Transform>();

    auto parent = entity;

    if (actor.id != Actor::Boat
        && actor.id != Actor::Lantern)
    {
        entity = m_gameScene.createEntity();
        auto& shadowTx = entity.addComponent<xy::Transform>();
        shadowTx.setOrigin(8.f, 8.f);
        shadowTx.setPosition(bounds.width / 2.f, 0.f);
        entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::PlaneShader));
        entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/player_shadow.png"));
        entity.addComponent<SimpleShadow>().parent = parent;
        //entity.addComponent<xy::CommandTarget>().ID = CommandID::BasicShadow;

        spriteTx.addChild(shadowTx);
    }

    //add a weapon sprite to players
    if (hasWeapon)
    {
        //water rings
        addRings(parent);

        parent.addComponent<xy::Callback>().function = RedFlashCallback();
        parent.getComponent<xy::CommandTarget>().ID |= CommandID::PlayerAvatar | CommandID::InventoryHolder;
        parent.addComponent<Inventory>();

        addHealthBar(parent, Global::PlayerColours[actor.id - Actor::ID::PlayerOne]);

        //weapon
        auto weaponBounds = m_sprites[SpriteID::WeaponJean].getTextureBounds();
        entity = m_gameScene.createEntity();
        entity.addComponent<Sprite3D>(m_modelMatrices).verticalOffset = weaponBounds.height / 2.f; //cos scale 0.5f
        entity.addComponent<xy::Sprite>() = (actor.id == Actor::ID::PlayerTwo) ? m_sprites[SpriteID::WeaponJean] : m_sprites[SpriteID::WeaponRodney];
        entity.addComponent<xy::SpriteAnimation>();
        entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Sprite>().getTexture());
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.addComponent<ClientWeapon>().parent = parent;
        entity.getComponent<ClientWeapon>().spriteOffset = entity.getComponent<Sprite3D>().verticalOffset;
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Weapon;
        auto& weaponTx = entity.addComponent<xy::Transform>();
        weaponTx.setOrigin(weaponBounds.width / 2.f, 0.f);
        weaponTx.setPosition(bounds.width / 2.f, 0.f);
        spriteTx.addChild(weaponTx);
        parent.getComponent<DepthComponent>().playerWeapon = entity;
    }


    auto* msg = getContext().appInstance.getMessageBus().post<ActorEvent>(MessageID::ActorMessage);
    msg->type = ActorEvent::Spawned;
    msg->position = position;
    msg->id = actor.id;
}