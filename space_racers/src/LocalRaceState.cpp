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

#include "LocalRaceState.hpp"
#include "Sprite3D.hpp"
#include "Camera3D.hpp"
#include "CameraTarget.hpp"
#include "VehicleSystem.hpp"
#include "AsteroidSystem.hpp"
#include "TrailSystem.hpp"
#include "InverseRotationSystem.hpp"
#include "GameConsts.hpp"
#include "AIDriverSystem.hpp"
#include "VehicleDefs.hpp"
#include "CommandIDs.hpp"
#include "MessageIDs.hpp"
#include "VFXDirector.hpp"
#include "TimeTrialDirector.hpp"
#include "WayPoint.hpp"
#include "VertexFunctions.hpp"
#include "NixieDisplay.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/gui/Gui.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>

namespace
{
#include "Sprite3DShader.inl"
#include "TrackShader.inl"
#include "GlobeShader.inl"
#include "VehicleShader.inl"
}

LocalRaceState::LocalRaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_backgroundScene   (ctx.appInstance.getMessageBus()),
    m_gameScene         (ctx.appInstance.getMessageBus()),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_uiSounds          (m_audioResource),
    m_mapParser         (m_gameScene),
    m_renderPath        (m_resources),
    m_playerInput       (sd.localPlayers[0].inputBinding),
    m_state             (Readying)
{
    launchLoadingScreen();
    initScene();
    loadResources();

    if (!loadMap())
    {
        sd.errorMessage = "Failed to load map";
        requestStackPush(StateID::Error);
    }

    buildUI();
    spawnVehicle();
    quitLoadingScreen();
}

//public
bool LocalRaceState::handleEvent(const sf::Event& evt)
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

void LocalRaceState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::GameMessage)
    {
        const auto& data = msg.getData<GameEvent>();
        if (data.type == GameEvent::RaceStarted)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Game::Vehicle;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Normal);
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
    else if (msg.id == MessageID::VehicleMessage)
    {
        const auto& data = msg.getData<VehicleEvent>();
        if (data.type == VehicleEvent::RequestRespawn)
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

            auto& vehicle = entity.getComponent<Vehicle>();
            auto& tx = entity.getComponent<xy::Transform>();

            vehicle.velocity = {};
            vehicle.anglularVelocity = {};
            vehicle.stateFlags = (1 << Vehicle::Normal);
            vehicle.invincibleTime = GameConst::InvincibleTime;

            if (vehicle.currentWaypoint.isValid())
            {
                tx.setPosition(vehicle.currentWaypoint.getComponent<xy::Transform>().getPosition());
                tx.setRotation(vehicle.currentWaypoint.getComponent<WayPoint>().rotation);
                auto& wp = vehicle.currentWaypoint.getComponent<WayPoint>();
            }
            else
            {
                auto [position, rotation] = m_mapParser.getStartPosition();
                tx.setPosition(position);
                tx.setRotation(rotation);
            }

            tx.setScale(1.f, 1.f);

            auto* respawnMsg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
            respawnMsg->type = VehicleEvent::Respawned;
            respawnMsg->entity = entity;
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
        else if (data.type == VehicleEvent::Fell)
        {
            auto entity = data.entity;
            entity.getComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth - 1);

            m_gameScene.getActiveCamera().getComponent<CameraTarget>().lockedOn = false;
        }
        else if (data.type == VehicleEvent::LapLine)
        {
            //count laps and end time trial when done
            m_sharedData.gameData.lapCount--;

            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::LapText;
            cmd.action = [&](xy::Entity e, float)
            {
                e.getComponent<Nixie>().lowerValue = m_sharedData.gameData.lapCount;
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            if (m_sharedData.gameData.lapCount == 0)
            {
                requestStackPush(StateID::Summary);
                m_playerInput.getPlayerEntity().getComponent<Vehicle>().stateFlags = (1 << Vehicle::Disabled);
            }
        }
    }

    m_backgroundScene.forwardMessage(msg);
    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool LocalRaceState::update(float dt)
{
    switch (m_state)
    {
    default: break;
    case Readying:
        if (m_stateTimer.getElapsedTime() > sf::seconds(3.f))
        {
            m_state = Counting;
            m_stateTimer.restart();

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
    case Counting:
        if (m_stateTimer.getElapsedTime() > sf::seconds(4.f))
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::StartLights;
            cmd.action = [](xy::Entity e, float dt)
            {
                e.getComponent<xy::Callback>().active = true;
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            auto* msg = getContext().appInstance.getMessageBus().post<GameEvent>(MessageID::GameMessage);
            msg->type = GameEvent::RaceStarted;

            m_state = Racing;
        }
        break;
    }

    static float shaderTime = 0.f;
    shaderTime += dt;
    m_shaders.get(ShaderID::Globe).setUniform("u_time", shaderTime / 100.f);
    m_shaders.get(ShaderID::Asteroid).setUniform("u_time", -shaderTime / 10.f);

    m_playerInput.update(dt);
    m_backgroundScene.update(dt);
    m_gameScene.update(dt);
    m_uiScene.update(dt);

    auto camPosition = m_gameScene.getActiveCamera().getComponent<xy::Transform>().getPosition();
    m_backgroundScene.getActiveCamera().getComponent<xy::Transform>().setPosition(camPosition);
    m_renderPath.updateView(camPosition);

    return true;
}

void LocalRaceState::draw()
{
    m_renderPath.render(m_backgroundScene, m_gameScene);

    auto& rw = getContext().renderWindow;
    rw.setView(getContext().defaultView);
    rw.draw(m_renderPath);

    rw.draw(m_uiScene);
}

//private
void LocalRaceState::initScene()
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
    m_gameScene.addSystem<AIDriverSystem>(mb);
    m_gameScene.addSystem<xy::AudioSystem>(mb);

    m_gameScene.addDirector<VFXDirector>(m_sprites);


    m_uiScene.addSystem<xy::CommandSystem>(mb);
    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<NixieSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::SpriteAnimator>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);
    m_uiScene.addSystem<xy::AudioSystem>(mb);

    auto view = getContext().defaultView;
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());
}

void LocalRaceState::loadResources()
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
    m_textureIDs[TextureID::Game::Fence] = m_resources.load<sf::Texture>("assets/images/fence.png");
    m_textureIDs[TextureID::Game::Chevron] = m_resources.load<sf::Texture>("assets/images/chevron.png");
    m_textureIDs[TextureID::Game::Barrier] = m_resources.load<sf::Texture>("assets/images/barrier.png");
    m_textureIDs[TextureID::Game::Pylon] = m_resources.load<sf::Texture>("assets/images/pylon.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Pylon]).setSmooth(true);
    m_textureIDs[TextureID::Game::Bollard] = m_resources.load<sf::Texture>("assets/images/bollard.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Bollard]).setSmooth(true);
    m_textureIDs[TextureID::Game::LapLine] = m_resources.load<sf::Texture>("assets/images/lapline.png");
    m_textureIDs[TextureID::Game::NixieSheet] = m_resources.load<sf::Texture>("assets/images/nixie_sheet.png");
    m_textureIDs[TextureID::Game::LapCounter] = m_resources.load<sf::Texture>("assets/images/laps.png");

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

    if (m_sharedData.fontID == 0)
    {
        m_sharedData.fontID = m_sharedData.resources.load<sf::Font>("assets/fonts/" + FontID::DefaultFont);
    }
}

bool LocalRaceState::loadMap()
{
    if (!m_mapParser.load("assets/maps/" + m_sharedData.mapName))
    {
        return false;
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
    camEnt.addComponent<xy::Camera>().setView(/*view.getSize()*/xy::DefaultSceneSize);
    //camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());
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

    LOG("ROIDS ROIDS ROIDS", xy::Logger::Type::Warning);

    return true;
}

void LocalRaceState::addProps()
{
    auto cameraEntity = m_gameScene.getActiveCamera();

    //electric fences
    const auto& fences = m_mapParser.getFences();
    const auto& fenceTexture = m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Fence]);
    auto texSize = sf::Vector2f(fenceTexture.getSize());

    for (const auto& f : fences)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>(); //points are in world space.
        entity.addComponent<xy::Drawable>().setFilterFlags(GameConst::FilterFlags::All);
        entity.addComponent<Lightning>() = f;

        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(f.start);
        entity.addComponent<xy::Drawable>().setDepth(1000);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().setTexture(&fenceTexture);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::FenceHeight;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createBillboard(f.start, f.end, entity.getComponent<Sprite3D>().depth, texSize);
        entity.getComponent<xy::Drawable>().updateLocalBounds();

    }

    //chevrons
    const auto& chevrons = m_mapParser.getChevrons();
    const auto& chevronTexture = m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Chevron]);
    texSize = sf::Vector2f(chevronTexture.getSize());

    for (const auto& [start, end] : chevrons)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(start);
        entity.addComponent<xy::Drawable>().setDepth(1000);
        entity.getComponent<xy::Drawable>().setTexture(&chevronTexture);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::ChevronHeight;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createBillboard(start, end, entity.getComponent<Sprite3D>().depth, texSize);
        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }

    //race barriers
    const auto& barriers = m_mapParser.getBarriers();
    auto& barrierTexture = m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Barrier]);
    barrierTexture.setRepeated(true);
    texSize = sf::Vector2f(barrierTexture.getSize());

    for (const auto& [start, end] : barriers)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(start);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
        entity.getComponent<xy::Drawable>().setTexture(&barrierTexture);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::BarrierHeight;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createBillboard(start, end, entity.getComponent<Sprite3D>().depth, { xy::Util::Vector::length(end - start), texSize.y });
        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }

    //electric pylons
    const auto& pylons = m_mapParser.getPylons();
    auto& pylonTexture = m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Pylon]);
    texSize = sf::Vector2f(pylonTexture.getSize());
    for (auto p : pylons)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(p);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().setTexture(&pylonTexture);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::PylonHeight;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createPylon(texSize);

        entity.getComponent<xy::Drawable>().updateLocalBounds();
    }


    //bollards.
    const auto& bollards = m_mapParser.getBollards();
    auto& bollardTexture = m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Bollard]);
    texSize = sf::Vector2f(bollardTexture.getSize());
    for (auto b : bollards)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(b);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth - 1);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().setTexture(&bollardTexture);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::BollardHeight;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        entity.getComponent<xy::Drawable>().getVertices() = createCylinder(GameConst::BollardRadius, texSize, GameConst::BollardHeight);

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

    texSize = sf::Vector2f(entity.getComponent<xy::Drawable>().getTexture()->getSize());
    entity.getComponent<xy::Drawable>().getVertices() = createStartField(texSize.x, entity.getComponent<Sprite3D>().depth);

    entity.getComponent<xy::Drawable>().updateLocalBounds();

    //lap line
    auto& lapTexture = m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapLine]);
    texSize = sf::Vector2f(lapTexture.getSize());

    sf::Vector2f offset = xy::Util::Vector::rotate({ GameConst::LapArchOffset, 0.f }, m_mapParser.getStartPosition().second);

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(m_mapParser.getStartPosition().first + offset);
    entity.getComponent<xy::Transform>().setRotation(m_mapParser.getStartPosition().second);
    entity.addComponent<xy::Drawable>().setDepth(1000);
    entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
    entity.getComponent<xy::Drawable>().setTexture(&lapTexture);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<Sprite3D>(m_matrixPool).depth = texSize.y / 2.f;
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

    entity.getComponent<xy::Drawable>().getVertices() = createLapLine(/*texSize*/);

    entity.getComponent<xy::Drawable>().updateLocalBounds();
}

void LocalRaceState::buildUI()
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

    //lap counter frame
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-1);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapCounter]));

    //lap counter
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(GameConst::LapCounterPosition);
    entity.addComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::NixieSheet]));
    entity.addComponent<Nixie>().lowerValue = m_sharedData.gameData.lapCount;
    entity.getComponent<Nixie>().digitCount = 2;
    entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::LapText;
}

void LocalRaceState::spawnVehicle()
{
    auto [position, rotation] = m_mapParser.getStartPosition();

    //spawn vehicle
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setRotation(rotation);
    entity.addComponent<InverseRotation>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Vehicle));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseMap");
    entity.getComponent<xy::Drawable>().bindUniform("u_specularMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleSpecular]));
    entity.getComponent<xy::Drawable>().bindUniform("u_neonMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleNeon]));
    entity.getComponent<xy::Drawable>().bindUniform("u_neonColour", GameConst::PlayerColour::Light[0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_lightRotationMatrix", entity.getComponent<InverseRotation>().matrix.getMatrix());
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Car + m_sharedData.localPlayers[0].vehicle];
    entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(m_sharedData.localPlayers[0].vehicle);
    entity.getComponent<Vehicle>().colourID = 0;
    //TODO we should probably get this from the server, but it might not matter
    //as the server is the final arbiter in laps counted anyway
    entity.getComponent<Vehicle>().waypointCount = m_mapParser.getWaypointCount();
    entity.getComponent<Vehicle>().client = false; //technically true but we want to be authorative as there's no server

    entity.addComponent<CollisionObject>().type = CollisionObject::Vehicle;
    entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Game::Vehicle;

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

    spawnTrail(entity, GameConst::PlayerColour::Light[0]);
}

void LocalRaceState::spawnTrail(xy::Entity parent, sf::Color colour)
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

void LocalRaceState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}