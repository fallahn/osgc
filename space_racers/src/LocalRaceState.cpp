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
#include "NixieDisplay.hpp"
#include "SkidEffectSystem.hpp"
#include "EngineAudioSystem.hpp"
#include "SoundEffectsDirector.hpp"
#include "LapDotSystem.hpp"
#include "SplitScreenDirector.hpp"

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
#include <xyginext/ecs/components/ParticleEmitter.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/AudioSystem.hpp>
#include <xyginext/ecs/systems/ParticleSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>

namespace
{
#include "Sprite3DShader.inl"
#include "TrackShader.inl"
#include "GlobeShader.inl"
#include "VehicleShader.inl"
#include "ShieldShader.inl"
#include "MenuShader.inl"

    std::array<sf::FloatRect, 2u> splitViewports = 
    { 
        sf::FloatRect(0.f, 0.f, 0.5f, 1.f),
        sf::FloatRect(0.5f, 0.f, 0.5f, 1.f)
    };

    std::array<sf::FloatRect, 4u> quadViewports =
    {
        sf::FloatRect(0.f, 0.f, 0.5f, 0.5f),
        sf::FloatRect(0.5f, 0.f, 0.5f, 0.5f),
        sf::FloatRect(0.f, 0.5f, 0.5f, 0.5f),
        sf::FloatRect(0.5f, 0.5f, 0.5f, 0.5f)
    };

    std::array<sf::Vector2f, 4u> lapCounterPositions =
    {
        sf::Vector2f(),
        sf::Vector2f(xy::DefaultSceneSize.x / 2.f, 0.f),
        sf::Vector2f(0.f, xy::DefaultSceneSize.y / 2.f),
        xy::DefaultSceneSize / 2.f
    };
}

LocalRaceState::LocalRaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_backgroundScene   (ctx.appInstance.getMessageBus()),
    m_gameScene         (ctx.appInstance.getMessageBus()),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_uiSounds          (m_audioResource),
    m_raceSounds        (m_audioResource),
    m_mapParser         (m_gameScene),
    m_renderPath        (m_resources, m_shaders),
    m_playerInputs      ({ sd.localPlayers[0].inputBinding, sd.localPlayers[1].inputBinding, sd.localPlayers[2].inputBinding, sd.localPlayers[3].inputBinding })
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

    auto pause = [&]()
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::Game::Audio | CommandID::Game::Vehicle;
        cmd.action = [](xy::Entity e, float)
        {
            if (e.getComponent<xy::AudioEmitter>().getStatus() == xy::AudioEmitter::Playing)
            {
                e.getComponent<xy::AudioEmitter>().pause();
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        //makes sure there's enough time to pause audio
        //by delaying the pause state by one frame
        auto* msg = getContext().appInstance.getMessageBus().post<StateEvent>(MessageID::StateMessage);
        msg->type = StateEvent::RequestPush;
        msg->id = StateID::Pause;
    };

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::Escape:
        case sf::Keyboard::P:
        case sf::Keyboard::Pause:
            pause();
            break;
        }
    }
    else if (evt.type == sf::Event::JoystickButtonReleased)
    {
        if (evt.joystickButton.button == 7)
        {
            pause();
        }
    }

    for (auto& ip : m_playerInputs)
    {
        ip.handleEvent(evt);
    }

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
            cmd.targetFlags = CommandID::Game::Trail;
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
            }
            else
            {
                auto [position, rotation] = m_mapParser.getStartPosition();
                tx.setPosition(position);
                tx.setRotation(rotation);
            }

            tx.setScale(1.f, 1.f);

            entity.getComponent<xy::AudioEmitter>().play();

            auto* respawnMsg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
            respawnMsg->type = VehicleEvent::Respawned;
            respawnMsg->entity = entity;
        }
        else if (data.type == VehicleEvent::Exploded)
        {
            //detatch the current trail
            auto entity = data.entity;

            xy::Command cmd;
            cmd.targetFlags = CommandID::Game::Trail;
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
        }
        else if (data.type == VehicleEvent::LapLine)
        {
            //count laps and disable vehicle when done
            auto id = data.entity.getComponent<Vehicle>().colourID;
            auto entity = data.entity;

            if (m_sharedData.localPlayers[id].lapCount)
            {
                m_sharedData.localPlayers[id].lapCount--;
            }

            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::LapText;
            cmd.action = [&, id, entity](xy::Entity e, float)
            {
                if (e.getComponent<std::int32_t>() == id)
                {
                    e.getComponent<Nixie>().lowerValue = m_sharedData.localPlayers[id].lapCount;
                }
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            if (m_sharedData.localPlayers[id].lapCount == 0)
            {
                entity.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Disabled);
            }
        }
    }
    else if (msg.id == xy::Message::StateMessage)
    {
        const auto& data = msg.getData<xy::Message::StateEvent>();
        if (data.type == xy::Message::StateEvent::Popped
            && data.id == StateID::Pause)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Game::Audio | CommandID::Game::Vehicle;
            cmd.action = [](xy::Entity e, float)
            {
                if (e.getComponent<xy::AudioEmitter>().getStatus() == xy::AudioEmitter::Paused)
                {
                    e.getComponent<xy::AudioEmitter>().play();
                }
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
    else if (msg.id == MessageID::StateMessage)
    {
        const auto& data = msg.getData<StateEvent>();
        switch (data.type)
        {
        default: break;
        case StateEvent::RequestPush:
            requestStackPush(data.id);
            break;
        }
    }

    m_backgroundScene.forwardMessage(msg);
    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool LocalRaceState::update(float dt)
{
    static float shaderTime = 0.f;
    shaderTime += dt;
    m_shaders.get(ShaderID::Globe).setUniform("u_time", shaderTime / 100.f);
    m_shaders.get(ShaderID::Asteroid).setUniform("u_time", -shaderTime / 10.f);

    for (auto& ip : m_playerInputs)
    {
        ip.update(dt);
    }

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
    m_gameScene.addSystem<SkidEffectSystem>(mb);
    m_gameScene.addSystem<EngineAudioSystem>(mb);
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
    m_gameScene.addSystem<xy::ParticleSystem>(mb);
    m_gameScene.addSystem<AIDriverSystem>(mb);
    m_gameScene.addSystem<xy::AudioSystem>(mb);

    m_gameScene.addDirector<VFXDirector>(m_sprites, m_resources);
    m_gameScene.addDirector<SFXDirector>(m_resources);
    m_gameScene.addDirector<SplitScreenDirector>(m_uiScene, m_sharedData);

    m_uiScene.addSystem<xy::CommandSystem>(mb);
    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<NixieSystem>(mb);
    m_uiScene.addSystem<LapDotSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::SpriteAnimator>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);

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
    m_textureIDs[TextureID::Game::LapProgress] = m_resources.load<sf::Texture>("assets/images/play_bar.png");
    m_textureIDs[TextureID::Game::LapPoint] = m_resources.load<sf::Texture>("assets/images/ui_point.png");
    m_textureIDs[TextureID::Game::Shield] = m_resources.load<sf::Texture>("assets/images/shield.png");
    m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Shield]).setRepeated(true);

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
    m_shaders.preload(ShaderID::Asteroid, GlobeFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Vehicle, VehicleVertex, VehicleFrag);
    m_shaders.preload(ShaderID::Trail, VehicleTrail, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Lightbar, LightbarFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Shield, ShieldFragment, sf::Shader::Fragment);

    //only set these once if we can help it - no access to uniform IDs
    //in SFML means lots of string look-ups setting uniforms :(
    auto& vehicleShader = m_shaders.get(ShaderID::Vehicle);
    vehicleShader.setUniform("u_normalMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleNormal]));

    m_raceSounds.loadFromFile("assets/sound/race.xas");

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
    camEnt.getComponent<xy::Camera>().lockRotation(true);
    camEnt.addComponent<CameraTarget>();
    camEnt.addComponent<xy::AudioListener>();

    //background entity isn't created until we know how many cameras will exist (see spawnVehicle())

    auto view = getContext().defaultView;
    float fov = Camera3D::calcFOV(view.getSize().y);
    float ratio = view.getSize().x / view.getSize().y;
    camEnt.addComponent<Camera3D>().projectionMatrix = glm::perspective(fov, ratio, GameConst::CamNear, Camera3D::depth + GameConst::CamFar);

    m_gameScene.setActiveCamera(camEnt);
    m_gameScene.setActiveListener(camEnt);

    //background stars / planets are also added to the corresponding scene based on cam count (below)

    m_mapParser.addProps(m_matrixPool, m_audioResource, m_shaders, m_resources, m_textureIDs);

    createRoids();

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::AudioEmitter>() = m_raceSounds.getEmitter("ambience0" + std::to_string(xy::Util::Random::value(1, 5)));
    entity.getComponent<xy::AudioEmitter>().play();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Game::Audio;

    return true;
}

void LocalRaceState::createRoids()
{
    sf::FloatRect bounds = { sf::Vector2f(), m_mapParser.getSize() };
    bounds.left -= 100.f;
    bounds.top -= 100.f;
    bounds.width += 200.f;
    bounds.height += 200.f;
    m_gameScene.getSystem<AsteroidSystem>().setMapSize(bounds);

    m_gameScene.getSystem<AsteroidSystem>().setSpawnPosition(m_mapParser.getStartPosition().first);

    //auto cameraEntity = m_gameScene.getActiveCamera();
    sf::BlendMode blendMode(sf::BlendMode::DstAlpha, sf::BlendMode::OneMinusSrcAlpha);

    auto positions = xy::Util::Random::poissonDiscDistribution(bounds, 1200, 8);
    for (auto position : positions)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::RoidDiffuse]));

        sf::Vector2f velocity =
        {
            xy::Util::Random::value(-1.f, 1.f),
            xy::Util::Random::value(-1.f, 1.f)
        };
        entity.addComponent<Asteroid>().setVelocity(xy::Util::Vector::normalise(velocity) * xy::Util::Random::value(200.f, 500.f));

        auto aabb = entity.getComponent<xy::Sprite>().getTextureBounds();
        auto radius = aabb.width / 2.f;
        entity.getComponent<xy::Transform>().setOrigin(radius, radius);
        auto scale = xy::Util::Random::value(0.5f, 2.5f);
        entity.getComponent<xy::Transform>().setScale(scale, scale);
        entity.getComponent<Asteroid>().setRadius(radius * scale);

        auto collisionVerts = createCollisionCircle(radius * 0.9f, { radius, radius });
        entity.addComponent<CollisionObject>().type = CollisionObject::Type::Roid;
        entity.getComponent<CollisionObject>().applyVertices(collisionVerts);

        entity.addComponent<xy::BroadphaseComponent>().setArea(aabb);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Asteroid);

        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Asteroid));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetNormal]));

        auto shadowEnt = m_gameScene.createEntity();
        shadowEnt.addComponent<xy::Transform>().setPosition(sf::Vector2f(-18.f, 18.f));
        shadowEnt.addComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth - 1);
        //shadowEnt.getComponent<xy::Drawable>().setBlendMode(blendMode);
        shadowEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::RoidShadow]));
        entity.getComponent<xy::Transform>().addChild(shadowEnt.getComponent<xy::Transform>());
    }
}

void LocalRaceState::createBackground(xy::Scene& scene)
{
    sf::FloatRect bounds(sf::Vector2f(), m_mapParser.getSize());
    bounds.left -= 1000.f;
    bounds.top -= 1000.f;
    bounds.width += 2000.f;
    bounds.height += 2000.f;

    std::array<std::size_t, 3u> IDs =
    {
        m_textureIDs[TextureID::Game::StarsNear],
        m_textureIDs[TextureID::Game::StarsMid],
        m_textureIDs[TextureID::Game::StarsFar],
    };
    for (auto i = 0; i < 3; ++i)
    {
        auto entity = scene.createEntity();
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
        //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &camEnt.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    }

    //planet
    auto entity = scene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    entity.addComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Globe));
    entity.getComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth - 2);
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetNormal]));
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::PlanetDiffuse]));

}

void LocalRaceState::buildUI()
{
    //startlights
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x / 2.f, 0.f);
    entity.addComponent<xy::Drawable>().setDepth(100);
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
    
    //all sounds have to be in the game scene for positioning to work correctly
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::Game::StartLights;
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
    entity.addComponent<std::int32_t>() = 0; //so we can ID this when there are multiple players

    //lapline
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y - GameConst::LapLineOffset);
    entity.addComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapProgress]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    float scale = xy::DefaultSceneSize.x / bounds.width;
    entity.getComponent<xy::Transform>().setScale(scale, 1.f);
}

void LocalRaceState::addLapPoint(xy::Entity vehicle, sf::Color colour)
{
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y - (GameConst::LapLineOffset - 8.f));
    entity.addComponent<xy::Drawable>().setDepth(1); //TODO each one needs own depth?
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Lightbar));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapPoint])).setColour(colour);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<LapDot>().parent = vehicle;
    entity.getComponent<LapDot>().trackLength = m_mapParser.getTrackLength();
}

void LocalRaceState::spawnVehicle()
{
    auto [position, rotation] = m_mapParser.getStartPosition();

    xy::EmitterSettings smokeSettings;
    smokeSettings.loadFromFile("assets/particles/skidpuff.xyp", m_resources);

    std::vector<xy::Entity> cameras; //this holds any camera ents created for human players

    //spawn vehicle
    for (auto i = 0u; i < 4u; ++i)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position);
        entity.getComponent<xy::Transform>().setRotation(rotation);

        sf::Transform offsetTransform;
        offsetTransform.rotate(rotation);
        auto offset = offsetTransform.transformPoint(GameConst::SpawnPositions[i]);
        entity.getComponent<xy::Transform>().move(offset);

        entity.addComponent<InverseRotation>();
        entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Vehicle));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseMap");
        entity.getComponent<xy::Drawable>().bindUniform("u_specularMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleSpecular]));
        entity.getComponent<xy::Drawable>().bindUniform("u_neonMap", m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::VehicleNeon]));
        entity.getComponent<xy::Drawable>().bindUniform("u_neonColour", GameConst::PlayerColour::Light[i]);
        entity.getComponent<xy::Drawable>().bindUniform("u_lightRotationMatrix", entity.getComponent<InverseRotation>().matrix.getMatrix());
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Car + m_sharedData.localPlayers[i].vehicle];
        entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(m_sharedData.localPlayers[i].vehicle);
        entity.getComponent<Vehicle>().colourID = i;
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
            entity.addComponent<xy::ParticleEmitter>().settings = smokeSettings;
            entity.addComponent<xy::AudioEmitter>() = m_raceSounds.getEmitter("car");
            {
                auto skidEntity = m_gameScene.createEntity();
                skidEntity.addComponent<xy::Transform>();
                skidEntity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth + 1);
                skidEntity.addComponent<SkidEffect>().parent = entity;
            }
            break;
        case Vehicle::Bike:
            entity.getComponent<Vehicle>().settings = Definition::bike;
            entity.getComponent<CollisionObject>().applyVertices(GameConst::BikePoints);
            entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::BikeSize);
            entity.addComponent<xy::ParticleEmitter>().settings = smokeSettings;
            entity.addComponent<xy::AudioEmitter>() = m_raceSounds.getEmitter("bike");
            {
                auto skidEntity = m_gameScene.createEntity();
                skidEntity.addComponent<xy::Transform>();
                skidEntity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth + 1);
                skidEntity.addComponent<SkidEffect>().parent = entity;
                skidEntity.getComponent<SkidEffect>().wheelCount = 1;
            }
            break;
        case Vehicle::Ship:
            entity.getComponent<Vehicle>().settings = Definition::ship;
            entity.getComponent<CollisionObject>().applyVertices(GameConst::ShipPoints);
            entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::ShipSize);
            entity.addComponent<xy::AudioEmitter>() = m_raceSounds.getEmitter("ship");
            break;
        }
        auto bounds = entity.getComponent<xy::BroadphaseComponent>().getArea();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width * GameConst::VehicleCentreOffset, bounds.height / 2.f);
        entity.getComponent<xy::AudioEmitter>().play();
        entity.addComponent<EngineAudio>();

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

        auto shieldEnt = m_gameScene.createEntity();
        shieldEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        shieldEnt.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth + 1);
        shieldEnt.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
        shieldEnt.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Shield));
        shieldEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        shieldEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::Shield]));
        bounds = shieldEnt.getComponent<xy::Sprite>().getTextureBounds();
        shieldEnt.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        shieldEnt.addComponent<xy::Callback>().active = true;
        shieldEnt.getComponent<xy::Callback>().function =
            [entity](xy::Entity e, float)
        {
            const auto& vehicle = entity.getComponent<Vehicle>();
            float amount = std::min(std::max(0.f, vehicle.invincibleTime / GameConst::InvincibleTime), 1.f);

            sf::Color c(255, 255, 255, static_cast<sf::Uint8>(amount * 255.f));
            e.getComponent<xy::Sprite>().setColour(c);
            e.getComponent<xy::Transform>().setScale(amount, amount);
            e.getComponent<xy::Transform>().setRotation(-entity.getComponent<xy::Transform>().getRotation());
        };

        entity.getComponent<xy::Transform>().addChild(shieldEnt.getComponent<xy::Transform>());

        m_sharedData.localPlayers[i].lapCount = m_sharedData.gameData.lapCount;

        if (m_sharedData.localPlayers[i].cpu)
        {
            entity.addComponent<AIDriver>().target = m_mapParser.getStartPosition().first;
            entity.getComponent<AIDriver>().skill = static_cast<AIDriver::Skill>(xy::Util::Random::value(AIDriver::Excellent, AIDriver::Bad));
        }
        else
        {
            m_playerInputs[i].setPlayerEntity(entity);
            cameras.push_back(entity);
        }
        //cameras.push_back(entity); //TODO move this to human only, above

        //this triggers trail creation
        auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
        msg->type = VehicleEvent::Respawned;
        msg->entity = entity;

        addLapPoint(entity, GameConst::PlayerColour::Light[i]);

        m_gameScene.getDirector<SplitScreenDirector>().addPlayerEntity(entity);
    }

    //see how many cameras we got for human players
    //if there's only one then we don't need to do anything.
    auto createCamera = [&]()->xy::Entity
    {
        xy::Entity entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>();
        entity.addComponent<xy::Camera>().lockRotation(true);
        entity.addComponent<CameraTarget>();
        entity.addComponent<Camera3D>();

        return entity;
    };

    auto addLapCounter = [&](std::int32_t id)
    {
        if (id > 0)
        {
            //lap counter frame
            auto entity = m_uiScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(lapCounterPositions[id]);
            entity.addComponent<xy::Drawable>().setDepth(-1);
            entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapCounter]));
            auto frameEnt = entity;

            //lap counter
            entity = m_uiScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(GameConst::LapCounterPosition);
            entity.addComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::NixieSheet]));
            entity.addComponent<Nixie>().lowerValue = m_sharedData.gameData.lapCount;
            entity.getComponent<Nixie>().digitCount = 2;
            entity.addComponent<xy::CommandTarget>().ID = CommandID::UI::LapText;
            entity.addComponent<std::int32_t>() = id; //so we can ID this when there are multiple players
            frameEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
        }
    };

    auto setListener = [&]()
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(m_mapParser.getSize() / 2.f);
        entity.addComponent<xy::AudioListener>().setDepth(1000.f);
        //entity.getComponent<xy::AudioListener>().setVolume(10.f);
        //if we make this louder we need to make the music quieter
        m_gameScene.setActiveListener(entity);
    };

    switch (cameras.size())
    {
    default: break;
    case 1:
    {
        //there's only one camera so draw fancy background
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

        createBackground(m_backgroundScene);

        auto camEnt = m_gameScene.getActiveCamera();
        camEnt.getComponent<xy::Transform>().addChild(backgroundEnt.getComponent<xy::Transform>());
        camEnt.getComponent<CameraTarget>().target = cameras[0];

        cameras.clear();
    }
    break;
    case 2:
    {
        //view split vertically
        sf::Vector2f viewSize(xy::DefaultSceneSize.x / 2.f, xy::DefaultSceneSize.y);
        float fov = Camera3D::calcFOV(viewSize.y);
        float ratio = viewSize.x / viewSize.y;

        for (auto i = 0u; i < cameras.size(); ++i)
        {
            auto camEnt = createCamera();
            camEnt.getComponent<Camera3D>().projectionMatrix = glm::perspective(fov, ratio, GameConst::CamNear, Camera3D::depth + GameConst::CamFar);
            camEnt.getComponent<xy::Camera>().setView(viewSize);
            camEnt.getComponent<xy::Camera>().setViewport(splitViewports[i]);
            camEnt.getComponent<CameraTarget>().target = cameras[i];
            camEnt.getComponent<CameraTarget>().lastTarget = cameras[i];
            cameras[i] = camEnt;

            addLapCounter(i);
        }

        //add some borders to the UI
        auto entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.addComponent<xy::Drawable>().setDepth(-10);
        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        verts.emplace_back(sf::Vector2f(-GameConst::SplitScreenBorderThickness, -xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);
        verts.emplace_back(sf::Vector2f(GameConst::SplitScreenBorderThickness, -xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);
        verts.emplace_back(sf::Vector2f(GameConst::SplitScreenBorderThickness, xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);
        verts.emplace_back(sf::Vector2f(-GameConst::SplitScreenBorderThickness, xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);
        entity.getComponent<xy::Drawable>().updateLocalBounds();

        createBackground(m_gameScene);
        setListener();
        break;
    }
    case 3:
    case 4:
        {
            //split into quads. for 3 players
            //the 4th camera gives a map overview.
            auto viewSize = xy::DefaultSceneSize / 1.5f;
            float fov = Camera3D::calcFOV(viewSize.y);
            float ratio = viewSize.x / viewSize.y;

            for (auto i = 0u; i < cameras.size(); ++i)
            {
                auto camEnt = createCamera();
                camEnt.getComponent<Camera3D>().projectionMatrix = glm::perspective(fov, ratio, GameConst::CamNear, Camera3D::depth + GameConst::CamFar);
                camEnt.getComponent<xy::Camera>().setView(viewSize);
                camEnt.getComponent<xy::Camera>().setViewport(quadViewports[i]);
                camEnt.getComponent<CameraTarget>().target = cameras[i];
                camEnt.getComponent<CameraTarget>().lastTarget = cameras[i];
                cameras[i] = camEnt;

                addLapCounter(i);
            }

            //map view
            if (cameras.size() == 3)
            {
                //ideally we want to move further away
                //but the 3D sprite system has a fixed cam distance...
                auto mapSize = m_mapParser.getSize();
                if (mapSize.x > mapSize.y)
                {
                    viewSize.x = mapSize.x + 1500.f;
                    viewSize.y = viewSize.x / ratio;
                }
                else
                {
                    viewSize.y = mapSize.y + 1500.f;
                    viewSize.x = viewSize.y * ratio;
                }

                fov = Camera3D::calcFOV(viewSize.y);

                auto camEnt = createCamera();
                camEnt.getComponent<xy::Transform>().setPosition(mapSize / 2.f);
                camEnt.getComponent<Camera3D>().projectionMatrix = glm::perspective(fov, ratio, GameConst::CamNear, Camera3D::depth + GameConst::CamFar);
                camEnt.getComponent<xy::Camera>().setView(viewSize);
                camEnt.getComponent<xy::Camera>().setViewport(quadViewports[3]);
                cameras.push_back(camEnt);
            }

            //add some borders to the UI
            auto entity = m_uiScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
            entity.addComponent<xy::Drawable>().setDepth(-10);
            auto& verts = entity.getComponent<xy::Drawable>().getVertices();
            verts.emplace_back(sf::Vector2f(-GameConst::SplitScreenBorderThickness, -xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);
            verts.emplace_back(sf::Vector2f(GameConst::SplitScreenBorderThickness, -xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);
            verts.emplace_back(sf::Vector2f(GameConst::SplitScreenBorderThickness, xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);
            verts.emplace_back(sf::Vector2f(-GameConst::SplitScreenBorderThickness, xy::DefaultSceneSize.y / 2.f), GameConst::SplitScreenBorderColour);

            verts.emplace_back(sf::Vector2f(-xy::DefaultSceneSize.x / 2.f, -GameConst::SplitScreenBorderThickness), GameConst::SplitScreenBorderColour);
            verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x / 2.f, -GameConst::SplitScreenBorderThickness), GameConst::SplitScreenBorderColour);
            verts.emplace_back(sf::Vector2f(xy::DefaultSceneSize.x / 2.f, GameConst::SplitScreenBorderThickness), GameConst::SplitScreenBorderColour);
            verts.emplace_back(sf::Vector2f(-xy::DefaultSceneSize.x / 2.f, GameConst::SplitScreenBorderThickness), GameConst::SplitScreenBorderColour);
            entity.getComponent<xy::Drawable>().updateLocalBounds();

            createBackground(m_gameScene);
            setListener();
        }
        break;

    }
    m_renderPath.setCameras(cameras);
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