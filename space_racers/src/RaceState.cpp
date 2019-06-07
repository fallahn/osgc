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

#include "RaceState.hpp"
#include "VehicleSystem.hpp"
#include "VehicleDefs.hpp"
#include "NetConsts.hpp"
#include "ClientPackets.hpp"
#include "ServerPackets.hpp"
#include "GameConsts.hpp"
#include "NetActor.hpp"
#include "ActorIDs.hpp"
#include "DeadReckoningSystem.hpp"
#include "CommandIDs.hpp"
#include "CameraTarget.hpp"
#include "MessageIDs.hpp"
#include "AnimationCallbacks.hpp"
#include "VFXDirector.hpp"
#include "Camera3D.hpp"
#include "Sprite3D.hpp"
#include "AsteroidSystem.hpp"
#include "LightningSystem.hpp"
#include "InverseRotationSystem.hpp"
#include "TrailSystem.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>
#include <xyginext/ecs/components/Text.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Random.hpp>

namespace
{
#include "Sprite3DShader.inl"
#include "TrackShader.inl"
#include "GlobeShader.inl"
#include "VehicleShader.inl"

    const sf::Time pingTime = sf::seconds(3.f);
}

RaceState::RaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_backgroundScene   (ctx.appInstance.getMessageBus()),
    m_gameScene         (ctx.appInstance.getMessageBus()),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_uiSounds          (m_audioResource),
    m_mapParser         (m_gameScene),
    m_renderPath        (m_resources),
    m_playerInput       (sd.localPlayers[0].inputBinding, sd.netClient.get())
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildWorld();
    buildUI();

    buildTest();

    quitLoadingScreen();
}

//public
bool RaceState::handleEvent(const sf::Event& evt)
{
    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::Escape:
        case sf::Keyboard::P:
        case sf::Keyboard::Pause:
            requestStackPush(StateID::Pause);
            break;
        }
    }

    m_playerInput.handleEvent(evt);
    m_backgroundScene.forwardEvent(evt);
    m_gameScene.forwardEvent(evt);
    m_uiScene.forwardEvent(evt);
    return true;
}

void RaceState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::VehicleMessage)
    {
        const auto& data = msg.getData<VehicleEvent>();
        if (data.type == VehicleEvent::Fell)
        {
            auto entity = data.entity;
            entity.getComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth - 1);

            m_gameScene.getActiveCamera().getComponent<CameraTarget>().lockedOn = false;
        }
        else if (data.type == VehicleEvent::RequestRespawn)
        {
            auto entity = data.entity;
            entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);

            xy::Command cmd;
            cmd.targetFlags = CommandID::Trail;
            cmd.action = [entity](xy::Entity e, float)
            {
                if (e.getComponent<Trail>().parent == entity)
                {
                    e.getComponent<Trail>().parent = {};
                }
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        else if (data.type == VehicleEvent::Exploded)
        {
            //detatch the current trail
            auto entity = data.entity;

            xy::Command cmd;
            cmd.targetFlags = CommandID::Trail;
            cmd.action = [entity](xy::Entity e, float)
            {
                if (e.getComponent<Trail>().parent == entity)
                {
                    e.getComponent<Trail>().parent = {};
                }
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        else if (data.type == VehicleEvent::Respawned)
        {
            spawnTrail(data.entity, GameConst::PlayerColour::Light[data.entity.getComponent<Vehicle>().colourID]);
        }
    }

    m_backgroundScene.forwardMessage(msg);
    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool RaceState::update(float dt)
{
    handlePackets();

    static float shaderTime = 0.f;
    shaderTime += dt;
    m_shaders.get(ShaderID::Globe).setUniform("u_time", shaderTime / 100.f);
    m_shaders.get(ShaderID::Asteroid).setUniform("u_time", -shaderTime / 10.f);

    //let the server know we're ready
    if (m_sharedData.gameData.actorCount == 0)
    {
        m_sharedData.netClient->sendPacket(PacketID::ClientReady, std::int8_t(0), xy::NetFlag::Reliable);
        m_sharedData.gameData.actorCount = std::numeric_limits<std::uint8_t>::max(); //just so this stops triggering
    }

    if (m_pingClock.getElapsedTime() > pingTime)
    {
        m_sharedData.netClient->sendPacket(PacketID::ClientPing, std::uint8_t(0), xy::NetFlag::Unreliable);
        m_pingClock.restart();
    }

    m_playerInput.update(dt);
    m_backgroundScene.update(dt);
    m_gameScene.update(dt);
    m_uiScene.update(dt);

    auto camPosition = m_gameScene.getActiveCamera().getComponent<xy::Transform>().getPosition();
    m_backgroundScene.getActiveCamera().getComponent<xy::Transform>().setPosition(camPosition);
    m_renderPath.updateView(camPosition);

    return true;
}

void RaceState::draw()
{
    m_renderPath.render(m_backgroundScene, m_gameScene);
    
    auto& rw = getContext().renderWindow;
    rw.setView(getContext().defaultView);
    rw.draw(m_renderPath);

    rw.draw(m_uiScene);
}

//private
void RaceState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_backgroundScene.addSystem<xy::SpriteSystem>(mb);
    m_backgroundScene.addSystem<Sprite3DSystem>(mb);
    m_backgroundScene.addSystem<xy::CameraSystem>(mb);
    m_backgroundScene.addSystem<Camera3DSystem>(mb);
    m_backgroundScene.addSystem<xy::RenderSystem>(mb);
    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setView(xy::DefaultSceneSize);
    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setViewport({ 0.f, 0.f, 1.f, 1.f });

    m_gameScene.addSystem<VehicleSystem>(mb);
    m_gameScene.addSystem<DeadReckoningSystem>(mb);
    m_gameScene.addSystem<AsteroidSystem>(mb);
    m_gameScene.addSystem<LightningSystem>(mb);
    m_gameScene.addSystem<InverseRotationSystem>(mb);
    m_gameScene.addSystem<TrailSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<Sprite3DSystem>(mb);
    m_gameScene.addSystem<CameraTargetSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<Camera3DSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
    m_gameScene.addSystem<xy::AudioSystem>(mb);
    
    m_gameScene.addDirector<VFXDirector>(m_sprites);

    m_uiScene.addSystem<xy::CommandSystem>(mb);
    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::SpriteAnimator>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);
    m_uiScene.addSystem<xy::AudioSystem>(mb);

    auto view = getContext().defaultView;
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());
}

void RaceState::loadResources()
{
    m_textureIDs[TextureID::Game::StarsFar] = m_resources.load<sf::Texture>("assets/images/stars_far.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::StarsFar]).setRepeated(true);
    m_textureIDs[TextureID::Game::StarsMid] = m_resources.load<sf::Texture>("assets/images/stars_mid.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::StarsMid]).setRepeated(true);
    m_textureIDs[TextureID::Game::StarsNear] = m_resources.load<sf::Texture>("assets/images/stars_near.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::StarsNear]).setRepeated(true);

    m_textureIDs[TextureID::Game::PlanetDiffuse] = m_resources.load<sf::Texture>("assets/images/globe_diffuse.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetDiffuse]).setRepeated(true);
    m_textureIDs[TextureID::Game::RoidDiffuse] = m_resources.load<sf::Texture>("assets/images/roid_diffuse.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::RoidDiffuse]).setRepeated(true);
    m_textureIDs[TextureID::Game::PlanetNormal] = m_resources.load<sf::Texture>("assets/images/crater_normal.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetNormal]).setRepeated(true);
    m_textureIDs[TextureID::Game::RoidShadow] = m_resources.load<sf::Texture>("assets/images/roid_shadow.png");
    m_textureIDs[TextureID::Game::VehicleNormal] = m_resources.load<sf::Texture>("assets/images/vehicles/vehicles_normal.png");
    m_textureIDs[TextureID::Game::VehicleSpecular] = m_resources.load<sf::Texture>("assets/images/vehicles/vehicles_specular.png");
    m_textureIDs[TextureID::Game::VehicleNeon] = m_resources.load<sf::Texture>("assets/images/vehicles/vehicles_neon.png");
    m_textureIDs[TextureID::Game::VehicleShadow] = m_resources.load<sf::Texture>("assets/images/vehicles/vehicles_shadow.png");
    m_textureIDs[TextureID::Game::VehicleTrail] = m_resources.load<sf::Texture>("assets/images/vehicles/trail.png");
    m_textureIDs[TextureID::Game::LapProgress] = m_resources.load<sf::Texture>("assets/images/play_bar.png");
    m_textureIDs[TextureID::Game::LapPoint] = m_resources.load<sf::Texture>("assets/images/ui_point.png");
    m_textureIDs[TextureID::Game::Fence] = m_resources.load<sf::Texture>("assets/images/fence.png");

    //init render path
    if (!m_renderPath.init(m_sharedData.useBloom))
    {
        m_sharedData.errorMessage = "Failed to create render buffer. Try lo spec rendering?";
        requestStackPush(StateID::Error);
        return;
    }

    xy::SpriteSheet spriteSheet;

    //game scene assets
    spriteSheet.loadFromFile("assets/sprites/dust_puff.spt", m_resources);
    m_sprites[SpriteID::Game::SmokePuff] = spriteSheet.getSprite("dust_puff");

    spriteSheet.loadFromFile("assets/sprites/explosion.spt", m_resources);
    m_sprites[SpriteID::Game::Explosion] = spriteSheet.getSprite("explosion");

    spriteSheet.loadFromFile("assets/sprites/vehicles.spt", m_resources);
    m_sprites[SpriteID::Game::Car] = spriteSheet.getSprite("car");
    m_sprites[SpriteID::Game::Bike] = spriteSheet.getSprite("bike");
    m_sprites[SpriteID::Game::Ship] = spriteSheet.getSprite("ship");

    m_shaders.preload(ShaderID::Sprite3DTextured, SpriteVertex, SpriteFragmentTextured);
    m_shaders.preload(ShaderID::Sprite3DColoured, SpriteVertex, SpriteFragmentColoured);
    m_shaders.preload(ShaderID::TrackDistortion, TrackFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Globe, GlobeFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Asteroid, SpriteVertex, GlobeFragment);
    m_shaders.preload(ShaderID::Vehicle, VehicleVertex, VehicleFrag);
    m_shaders.preload(ShaderID::Trail, VehicleTrail, sf::Shader::Fragment);

    //only set these once if we can help it - no access to uniform IDs
    //in SFML means lots of string look-ups setting uniforms :(
    auto& vehicleShader = m_shaders.get(ShaderID::Vehicle);
    vehicleShader.setUniform("u_normalMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleNormal]));


    //ui scene assets
    spriteSheet.loadFromFile("assets/sprites/lights.spt", m_resources);
    m_sprites[SpriteID::Game::UIStartLights] = spriteSheet.getSprite("lights");

    m_uiSounds.loadFromFile("assets/sound/ui.xas");
}

void RaceState::buildWorld()
{
    if (!m_mapParser.load("assets/maps/" + m_sharedData.mapName))
    {
        m_sharedData.errorMessage = "Client was unable to load map";
        requestStackClear();
        requestStackPush(StateID::Error);
        m_sharedData.netClient->disconnect();
        return;
    }

    sf::FloatRect bounds(sf::Vector2f(), m_mapParser.getSize());
    bounds.left -= 1000.f;
    bounds.top -= 1000.f;
    bounds.width += 2000.f;
    bounds.height += 2000.f;
    m_gameScene.getSystem<AsteroidSystem>().setMapSize(bounds);
    m_gameScene.getSystem<AsteroidSystem>().setSpawnPosition(m_mapParser.getStartPosition().first);

    m_mapParser.renderLayers(m_trackTextures);
    m_renderPath.setNormalTexture(m_trackTextures[GameConst::TrackLayer::Normal].getTexture());

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth);
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.addComponent<xy::Sprite>(m_trackTextures[GameConst::TrackLayer::Track].getTexture());

    //add a camera   
    auto camEnt = m_gameScene.createEntity();
    camEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
    camEnt.addComponent<xy::Camera>().setView(xy::DefaultSceneSize);
    camEnt.getComponent<xy::Camera>().lockRotation(true);
    camEnt.addComponent<CameraTarget>();
    camEnt.addComponent<xy::AudioListener>();

    auto backgroundEnt = m_gameScene.createEntity();
    backgroundEnt.addComponent<xy::Transform>().setOrigin(sf::Vector2f(GameConst::LargeBufferSize) / 2.f);
    backgroundEnt.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundRenderDepth);
    backgroundEnt.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    if (m_sharedData.useBloom)
    {
        backgroundEnt.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::TrackDistortion));
        backgroundEnt.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_renderPath.getNormalBuffer());
        backgroundEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    }
    backgroundEnt.addComponent<xy::Sprite>(m_renderPath.getBackgroundBuffer());
    camEnt.getComponent<xy::Transform>().addChild(backgroundEnt.getComponent<xy::Transform>());

    auto view = getContext().defaultView;
    float fov = Camera3D::calcFOV(view.getSize().y);
    float ratio = view.getSize().x / view.getSize().y;
    camEnt.addComponent<Camera3D>().projectionMatrix = glm::perspective(fov, ratio, 10.f, Camera3D::depth + 2600.f);

    m_gameScene.setActiveCamera(camEnt);
    m_gameScene.setActiveListener(camEnt);

    //dem starrs
    std::array<std::size_t, 3u> IDs =
    {
        m_textureIDs[TextureID::Game::StarsNear],
        m_textureIDs[TextureID::Game::StarsMid],
        m_textureIDs[TextureID::Game::StarsFar],
    };
    for (auto i = 0; i < 3; ++i)
    {
        entity = m_backgroundScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(bounds.left, bounds.top);
        entity.getComponent<xy::Transform>().setScale(4.f, 4.f);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth - (2 + i));
        entity.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
        entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(IDs[i]));

        auto& verts = entity.getComponent<xy::Drawable>().getVertices();

        verts.emplace_back(sf::Vector2f(bounds.left, bounds.top), sf::Vector2f(bounds.left, bounds.top));
        verts.emplace_back(sf::Vector2f(bounds.left + bounds.width, bounds.top), sf::Vector2f(bounds.left + bounds.width, bounds.top));
        verts.emplace_back(sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height), sf::Vector2f(bounds.left + bounds.width, bounds.top + bounds.height));
        verts.emplace_back(sf::Vector2f(bounds.left, bounds.top + bounds.height), sf::Vector2f(bounds.left, bounds.top + bounds.height));

        entity.getComponent<xy::Drawable>().updateLocalBounds();


        entity.addComponent<Sprite3D>(m_matrixPool).depth = -(1500.f + (i * 500.f));
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &camEnt.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    }

    //planet
    entity = m_backgroundScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Globe));
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetNormal]));
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetDiffuse]));

    addProps();

    //let the server know the world is loaded so it can send us all player cars
    m_sharedData.netClient->sendPacket(PacketID::ClientMapLoaded, std::uint8_t(0), xy::NetFlag::Reliable);
}

void RaceState::addProps()
{
    const auto& barriers = m_mapParser.getBarriers();
    auto cameraEntity = m_gameScene.getActiveCamera();

    const auto& barrierTexture = m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Fence]);
    auto texSize = sf::Vector2f(barrierTexture.getSize());

    for (const auto& b : barriers)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>(); //points are in world space.
        entity.addComponent<xy::Drawable>().setFilterFlags(GameConst::FilterFlags::All);
        entity.addComponent<Lightning>() = b;

        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(b.start);
        entity.addComponent<xy::Drawable>().setDepth(1000);
        entity.getComponent<xy::Drawable>().setTexture(&barrierTexture);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool).depth = 40.f;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        verts.emplace_back(sf::Vector2f(), sf::Color::White, sf::Vector2f());
        verts.emplace_back(sf::Vector2f(b.end - b.start), sf::Color::White, sf::Vector2f(texSize.x, 0.f));
        verts.emplace_back(sf::Vector2f(b.end - b.start), sf::Color::Black, texSize);
        verts.emplace_back(sf::Vector2f(), sf::Color::Black, sf::Vector2f(0.f, texSize.y));

        entity.getComponent<xy::Drawable>().updateLocalBounds();

    }

    auto temp = m_resources.load<sf::Texture>("assets/images/start_field.png");
    m_resources.get<sf::Texture>(temp).setSmooth(true);
    
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_mapParser.getStartPosition().first);
    entity.addComponent<xy::Drawable>().setDepth(1000);
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(temp));
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<Sprite3D>(m_matrixPool).depth = 50.f;
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    sf::Color c(0, 0, 0);
    texSize = sf::Vector2f(entity.getComponent<xy::Drawable>().getTexture()->getSize());

    for (auto i = 0; i <= 255; i += 85)
    {
        c.r += i;

        verts.emplace_back(-texSize, c, sf::Vector2f(0.f, 0.f));
        verts.emplace_back(sf::Vector2f(texSize.x, -texSize.y), c, sf::Vector2f(texSize.x, 0.f));
        verts.emplace_back(texSize, c, texSize);
        verts.emplace_back(sf::Vector2f(-texSize.x, texSize.y), c, sf::Vector2f(0.f, texSize.y));
    }

    entity.getComponent<xy::Drawable>().updateLocalBounds();
}

void RaceState::buildUI()
{
    //startlights
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 0.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::UIStartLights];
    entity.addComponent<xy::SpriteAnimation>();
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::StartLights;
    entity.addComponent<xy::Callback>().userData = std::make_any<float>(1.5f);
    entity.getComponent<xy::Callback>().function = [](xy::Entity e, float dt)
    {
        auto& delay = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        delay -= dt;

        if (delay < 0)
        {
            e.getComponent<xy::Transform>().move(0.f, -100.f * dt);
            if (e.getComponent<xy::Transform>().getPosition().y < -e.getComponent<xy::Sprite>().getTextureBounds().height)
            {
                e.getComponent<xy::Callback>().active = false;
            }
        }
    };
    entity.addComponent<xy::AudioEmitter>() = m_uiSounds.getEmitter("start");

    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);

    //lap counter
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(40.f, 60.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setString("Laps: " + std::to_string(m_sharedData.gameData.lapCount));
    entity.getComponent<xy::Text>().setCharacterSize(36);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::LapText;

    //lapline
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y - GameConst::LapLineOffset);
    entity.addComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapProgress]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    float scale = xy::DefaultSceneSize.x / bounds.width;
    entity.getComponent<xy::Transform>().setScale(scale, 1.f);
}

void RaceState::addLapPoint(xy::Entity vehicle, sf::Color colour)
{
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y - (GameConst::LapLineOffset - 8.f));
    entity.addComponent<xy::Drawable>().setDepth(1); //TODO each one needs own depth?
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapPoint])).setColour(colour);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [&,vehicle](xy::Entity e, float)
    {
        auto position = e.getComponent<xy::Transform>().getPosition();
        position.x = (vehicle.getComponent<Vehicle>().totalDistance / m_mapParser.getTrackLength()) * xy::DefaultSceneSize.x;
        e.getComponent<xy::Transform>().setPosition(position);
    };
}

void RaceState::buildTest()
{

}

void RaceState::handlePackets()
{
    //poll event afterwards so gathered inputs are sent immediately
    xy::NetEvent evt;
    while (m_sharedData.netClient->pollEvent(evt))
    {
        if (evt.type == xy::NetEvent::PacketReceived)
        {
            const auto& packet = evt.packet;
            switch (packet.getID())
            {
            default: break;
            case PacketID::LapLine:
                updateLapLine(packet.as<std::uint32_t>());
                break;
            case PacketID::RaceTimerStarted:
                showTimer();
                break;
            case PacketID::RaceFinished:
                m_playerInput.getPlayerEntity().getComponent<Vehicle>().stateFlags = (1 << Vehicle::Disabled);
                m_sharedData.racePositions = packet.as<std::array<std::uint64_t, 4u>>();
                requestStackPush(StateID::Summary);
                break;
            case PacketID::CountdownStarted:
            {
                xy::Command cmd;
                cmd.targetFlags = CommandID::UI::StartLights;
                cmd.action = [](xy::Entity e, float dt) 
                {
                    e.getComponent<xy::SpriteAnimation>().play(0); 
                    e.getComponent<xy::AudioEmitter>().play();
                };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
                break;
            case PacketID::RaceStarted:
                m_playerInput.getPlayerEntity().getComponent<Vehicle>().stateFlags = (1 << Vehicle::Normal);
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::UI::StartLights;
                    cmd.action = [](xy::Entity e, float dt)
                    {
                        e.getComponent<xy::Callback>().active = true; 
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
                break;
            case PacketID::VehicleExploded:
                explodeNetVehicle(packet.as<std::uint32_t>());                
                break;
            case PacketID::VehicleFell:
                fallNetVehicle(packet.as<std::uint32_t>());                
                break;
            case PacketID::VehicleSpawned:
                resetNetVehicle(packet.as<VehicleData>());
                break;
            case PacketID::ClientLeftRace:
            {
                PlayerIdent ident = packet.as<PlayerIdent>();
                removeNetVehicle(ident.serverID); 
                m_sharedData.playerInfo.erase(ident.peerID);
                LOG("Removed player at peer " + std::to_string(ident.peerID), xy::Logger::Type::Info);
            }
                break;
            case PacketID::VehicleData:
                spawnVehicle(packet.as<VehicleData>());
                break;
            case PacketID::ActorData:
                spawnActor(packet.as<ActorData>());
                break;
            case PacketID::ActorUpdate:
                updateActor(packet.as<ActorUpdate>());
                break;
            case PacketID::VehicleActorUpdate:
                updateActor(packet.as<VehicleActorUpdate>());
                break;
            case PacketID::DebugPosition:
                /*if (debugEnt.isValid())
                {
                    debugEnt.getComponent<xy::Transform>().setPosition(packet.as<sf::Vector2f>());
                }*/
                break;
            case PacketID::ClientUpdate:
                reconcile(packet.as<ClientUpdate>());
                break;
            case PacketID::ErrorServerGeneric:
                m_sharedData.errorMessage = "Server Error.";
                m_sharedData.netClient->disconnect();
                requestStackPush(StateID::Error);
                break;
            case PacketID::ErrorServerDisconnect:
                m_sharedData.errorMessage = "Host Disconnected.";
                m_sharedData.netClient->disconnect();
                requestStackPush(StateID::Error);
                break;
            }
        }
    }
}

void RaceState::spawnVehicle(const VehicleData& data)
{
    //spawn vehicle
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(data.x, data.y);
    entity.getComponent<xy::Transform>().setRotation(data.rotation);
    entity.addComponent<InverseRotation>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Vehicle));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseMap");
    entity.getComponent<xy::Drawable>().bindUniform("u_specularMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleSpecular]));
    entity.getComponent<xy::Drawable>().bindUniform("u_neonMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleNeon]));
    entity.getComponent<xy::Drawable>().bindUniform("u_neonColour", GameConst::PlayerColour::Light[data.colourID]);
    entity.getComponent<xy::Drawable>().bindUniform("u_lightRotationMatrix", entity.getComponent<InverseRotation>().matrix.getMatrix());
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Car + data.vehicleType];
    entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(data.vehicleType);
    entity.getComponent<Vehicle>().colourID = data.colourID;
    //TODO we should probably get this from the server, but it might not matter
    //as the server is the final arbiter in laps counted anyway
    entity.getComponent<Vehicle>().waypointCount = m_mapParser.getWaypointCount();
    entity.getComponent<Vehicle>().client = true;
    entity.addComponent<std::int32_t>() = data.serverID;

    entity.addComponent<CollisionObject>().type = CollisionObject::Vehicle;
    entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);

    switch (entity.getComponent<Vehicle>().type)
    {
    default:
    case Vehicle::Car:
        entity.getComponent<Vehicle>().settings = Definition::car;
        entity.getComponent<CollisionObject>().applyVertices(GameConst::CarPoints);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);
        break;
    case Vehicle::Bike:
        entity.getComponent<Vehicle>().settings = Definition::bike;
        entity.getComponent<CollisionObject>().applyVertices(GameConst::BikePoints);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::BikeSize);
        break;
    case Vehicle::Ship:
        entity.getComponent<Vehicle>().settings = Definition::ship;
        entity.getComponent<CollisionObject>().applyVertices(GameConst::ShipPoints);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::ShipSize);
        break;
    }
    auto bounds = entity.getComponent<xy::BroadphaseComponent>().getArea();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width * GameConst::VehicleCentreOffset, bounds.height / 2.f);

    auto shadowEnt = m_gameScene.createEntity();
    shadowEnt.addComponent<xy::Transform>().setOrigin(entity.getComponent<xy::Transform>().getOrigin());
    shadowEnt.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 2); //make sure renders below trail
    shadowEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleShadow]));
    shadowEnt.getComponent<xy::Sprite>().setTextureRect(entity.getComponent<xy::Sprite>().getTextureRect());
    shadowEnt.addComponent<xy::Callback>().active = true;
    shadowEnt.getComponent<xy::Callback>().function =
        [entity](xy::Entity thisEnt, float)
    {
        auto& thisTx = thisEnt.getComponent<xy::Transform>();
        const auto& thatTx = entity.getComponent<xy::Transform>();

        thisTx.setPosition(thatTx.getPosition() + sf::Vector2f(-10.f, 10.f)); //TODO larger shadow dist for ships
        thisTx.setRotation(thatTx.getRotation());
        thisTx.setScale(thatTx.getScale());
    };

    m_playerInput.setPlayerEntity(entity);

    auto cameraEntity = m_gameScene.getActiveCamera();
    cameraEntity.getComponent<CameraTarget>().target = entity;

    spawnTrail(entity, GameConst::PlayerColour::Light[data.colourID]);
    addLapPoint(entity, GameConst::PlayerColour::Light[data.colourID]);

    //count spawned vehicles and tell server when all are spawned
    m_sharedData.gameData.actorCount--;
}

void RaceState::spawnActor(const ActorData& data)
{
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(data.x, data.y);
    entity.getComponent<xy::Transform>().setRotation(data.rotation);
    entity.getComponent<xy::Transform>().setScale(data.scale, data.scale);
    entity.addComponent<DeadReckon>().prevTimestamp = data.timestamp; //important, initial reckoning will be way off otherwise
    entity.addComponent<NetActor>().actorID = data.actorID;
    entity.getComponent<NetActor>().serverID = data.serverID;
    entity.addComponent<xy::Drawable>();
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::All);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::NetActor;

    switch (data.actorID)
    {
    default:
        //TODO add default sprite
        break;
    case ActorID::Car:
    {
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Car];
        entity.getComponent<xy::Transform>().setOrigin(GameConst::CarSize.width * GameConst::VehicleCentreOffset, GameConst::CarSize.height / 2.f);       
        entity.addComponent<CollisionObject>().applyVertices(GameConst::CarPoints);       
        entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);
        entity.addComponent<Vehicle>().type = Vehicle::Car;

        goto Vehicle;
    }
        break;
    case ActorID::Bike:
    {        
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Bike];
        entity.getComponent<xy::Transform>().setOrigin(GameConst::BikeSize.width * GameConst::VehicleCentreOffset, GameConst::BikeSize.height / 2.f);
        entity.addComponent<CollisionObject>().applyVertices(GameConst::BikePoints);       
        entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::BikeSize);
        entity.addComponent<Vehicle>().type = Vehicle::Bike;

        goto Vehicle;
    }
        break;
    case ActorID::Ship:
    {
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Ship];
        entity.getComponent<xy::Transform>().setOrigin(GameConst::ShipSize.width * GameConst::VehicleCentreOffset, GameConst::ShipSize.height / 2.f);
        entity.addComponent<CollisionObject>().applyVertices(GameConst::ShipPoints);
        entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::ShipSize);
        entity.addComponent<Vehicle>().type = Vehicle::Ship;

        goto Vehicle;
    }
        break;

    Vehicle:
        entity.getComponent<CollisionObject>().type = CollisionObject::Type::Vehicle;
        entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
        entity.addComponent<InverseRotation>();

        entity.getComponent<Vehicle>().colourID = data.colourID;
        entity.getComponent<Vehicle>().waypointCount = m_mapParser.getWaypointCount();
        entity.getComponent<Vehicle>().client = true;

        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Vehicle));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseMap");
        entity.getComponent<xy::Drawable>().bindUniform("u_specularMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleSpecular]));
        entity.getComponent<xy::Drawable>().bindUniform("u_neonMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleNeon]));
        entity.getComponent<xy::Drawable>().bindUniform("u_neonColour", GameConst::PlayerColour::Light[data.colourID]);
        entity.getComponent<xy::Drawable>().bindUniform("u_lightRotationMatrix", entity.getComponent<InverseRotation>().matrix.getMatrix());

        spawnTrail(entity, GameConst::PlayerColour::Light[data.colourID]);
        addLapPoint(entity, GameConst::PlayerColour::Light[data.colourID]);

        {
            auto shadowEnt = m_gameScene.createEntity();
            shadowEnt.addComponent<xy::Transform>().setOrigin(entity.getComponent<xy::Transform>().getOrigin());
            shadowEnt.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
            shadowEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleShadow]));
            shadowEnt.getComponent<xy::Sprite>().setTextureRect(entity.getComponent<xy::Sprite>().getTextureRect());
            shadowEnt.addComponent<xy::Callback>().active = true;
            shadowEnt.getComponent<xy::Callback>().function =
                [entity](xy::Entity thisEnt, float)
            {
                auto& thisTx = thisEnt.getComponent<xy::Transform>();
                const auto& thatTx = entity.getComponent<xy::Transform>();

                thisTx.setPosition(thatTx.getPosition() + sf::Vector2f(-10.f, 10.f)); //TODO larger shadow dist for ships
                thisTx.setRotation(thatTx.getRotation());
                thisTx.setScale(thatTx.getScale());
            };

            auto debugEnt = m_gameScene.createEntity();
            debugEnt.addComponent<xy::Transform>();
            debugEnt.addComponent<xy::Drawable>().setDepth(1000);
            debugEnt.addComponent<xy::Callback>().active = true;
            debugEnt.getComponent<xy::Callback>().function =
                [entity](xy::Entity, float)
            {
                const auto& tx = entity.getComponent<xy::Transform>();
                DPRINT("Position", std::to_string(tx.getPosition().x) + ", " + std::to_string(tx.getPosition().y));
                DPRINT("Scale", std::to_string(tx.getScale().x) + ", " + std::to_string(tx.getScale().y));
            };
        }

        break;

    case ActorID::Roid:
    {
        entity.addComponent<CollisionObject>().type = CollisionObject::Type::NetActor;
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::RoidDiffuse]));
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        auto radius = bounds.width / 2.f;
        entity.getComponent<xy::Transform>().setOrigin(radius, radius);
        entity.getComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth);
        entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
        entity.addComponent<Asteroid>().setRadius(radius * data.scale);
        
        entity.getComponent<CollisionObject>().applyVertices(createCollisionCircle(radius * 0.9f, { radius, radius }));
        entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Asteroid);
        entity.getComponent<xy::BroadphaseComponent>().setArea(bounds);

        auto cameraEntity = m_gameScene.getActiveCamera();
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Asteroid));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetNormal]));
        entity.addComponent<Sprite3D>(m_matrixPool).depth = radius * data.scale;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        //TODO I'd quite like to mask these with the track alpha channel but.. meh. Maybe later.
        auto shadowEnt = m_gameScene.createEntity();
        shadowEnt.addComponent<xy::Transform>().setPosition(sf::Vector2f(-18.f, 18.f));
        shadowEnt.addComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth - 1);
        shadowEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::RoidShadow]));
        entity.getComponent<xy::Transform>().addChild(shadowEnt.getComponent<xy::Transform>());
    }
        break;
    }

    //count actors so we can tell the server when we're ready
    m_sharedData.gameData.actorCount--;
}

void RaceState::updateActor(const VehicleActorUpdate& update)
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::NetActor;
    cmd.action = [update](xy::Entity e, float)
    {
        auto& actor = e.getComponent<NetActor>();
        if (actor.serverID == update.serverID)
        {
            e.getComponent<DeadReckon>().update = update;
            e.getComponent<DeadReckon>().hasUpdate = true;
            actor.velocity = { update.velX, update.velY }; //this is used in client side collisions
        }
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void RaceState::reconcile(const ClientUpdate& update)
{
    auto entity = m_playerInput.getPlayerEntity();
    if (entity.isValid())
    {
        m_gameScene.getSystem<VehicleSystem>().reconcile(update, entity);
    }
}

void RaceState::resetNetVehicle(const VehicleData& data)
{
    if (auto e = m_playerInput.getPlayerEntity(); e.getComponent<std::int32_t>() == data.serverID)
    {
        auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
        msg->type = VehicleEvent::Respawned;
        msg->entity = e;
    }
    else
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::NetActor;
        cmd.action = [&,data](xy::Entity e, float)
        {
            if (e.getComponent<NetActor>().serverID == data.serverID)
            {
                /*e.getComponent<xy::Callback>().active = false;*/
                e.getComponent<xy::Transform>().setPosition(data.x, data.y);
                e.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
                e.getComponent<xy::Transform>().setScale(1.f, 1.f);

                auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
                msg->type = VehicleEvent::Respawned;
                msg->entity = e;
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void RaceState::explodeNetVehicle(std::uint32_t id)
{
    if (auto e = m_playerInput.getPlayerEntity(); e.getComponent<std::int32_t>() == id)
    {
        auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
        msg->type = VehicleEvent::Exploded;
        msg->entity = e;
    }
    else
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::NetActor;
        cmd.action = [&, id](xy::Entity e, float)
        {
            if (e.getComponent<NetActor>().serverID == id)
            {
                //e.getComponent<xy::Transform>().setScale(0.f, 0.f);

                auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
                msg->type = VehicleEvent::Exploded;
                msg->entity = e;
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void RaceState::fallNetVehicle(std::uint32_t id)
{
    /*if (auto e = m_playerInput.getPlayerEntity(); e.getComponent<std::int32_t>() == id)
    {
        auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
        msg->type = VehicleEvent::Fell;
        msg->entity = e;
    }
    else*/
    //above is already handled locally
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::NetActor;
        cmd.action = [&, id](xy::Entity e, float)
        {
            if (e.getComponent<NetActor>().serverID == id)
            {
                //e.getComponent<xy::Callback>().active = true; //performs scaling
                e.getComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth - 2);

                auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
                msg->type = VehicleEvent::Fell;
                msg->entity = e;

                /*xy::Command cmd2;
                cmd2.targetFlags = CommandID::Trail;
                cmd2.action = [e](xy::Entity ent, float)
                {
                    if (ent.getComponent<Trail>().parent == e)
                    {
                        ent.getComponent<Trail>().parent = {};
                    }
                };
                m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd2);*/
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void RaceState::removeNetVehicle(std::uint32_t id)
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::NetActor;
    cmd.action = [&, id](xy::Entity e, float)
    {
        if (e.getComponent<NetActor>().serverID == id)
        {
            m_gameScene.destroyEntity(e);
        }
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void RaceState::showTimer()
{
    auto& font = m_sharedData.resources.get<sf::Font>(m_sharedData.fontID);

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 40.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Text>(font).setFillColour(sf::Color::Red);
    entity.getComponent<xy::Text>().setCharacterSize(48);
    entity.getComponent<xy::Text>().setAlignment(xy::Text::Alignment::Centre);
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().userData = std::make_any<float>(60.f);
    entity.getComponent<xy::Callback>().function =
        [](xy::Entity e, float dt)
    {
        auto& currTime = std::any_cast<float&>(e.getComponent<xy::Callback>().userData);
        currTime = std::max(0.f, currTime - dt);
        e.getComponent<xy::Text>().setString(std::to_string(static_cast<std::int32_t>(currTime)));
    };
}

void RaceState::spawnTrail(xy::Entity parent, sf::Color colour)
{
    auto trailEnt = m_gameScene.createEntity();
    trailEnt.addComponent<xy::Transform>();
    trailEnt.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
    trailEnt.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleTrail]));
    trailEnt.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Trail));
    trailEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    trailEnt.addComponent<Trail>().parent = parent;
    trailEnt.getComponent<Trail>().colour = colour;
    trailEnt.addComponent<xy::CommandTarget>().ID = CommandID::Game::Trail;
}

void RaceState::updateLapLine(std::uint32_t serverID)
{
    if (serverID == static_cast<std::uint32_t>(m_playerInput.getPlayerEntity().getComponent<std::int32_t>()))
    {
        m_sharedData.gameData.lapCount--;

        xy::Command cmd;
        cmd.targetFlags = CommandID::LapText;
        cmd.action = [&](xy::Entity e, float)
        {
            e.getComponent<xy::Text>().setString("Laps: " + std::to_string(m_sharedData.gameData.lapCount));
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }

    //TODO play a sound when any vehicle crosses the line
}

void RaceState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}