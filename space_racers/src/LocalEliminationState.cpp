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

#include "LocalEliminationState.hpp"
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
#include "AIDriverSystem.hpp"
#include "SkidEffectSystem.hpp"
#include "EliminationDirector.hpp"
#include "LapDotSystem.hpp"
#include "EliminationDotSystem.hpp"

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
#include "MenuShader.inl"

    sf::Time CelebrationTime = sf::seconds(3.f);

    //arranged so that players in a lower position
    //are respawned further forward
    std::array<sf::Vector2f, 4u> SpawnOffsets =
    {
        sf::Vector2f(-GameConst::ShipSize.width / 2.f, -GameConst::ShipSize.height / 2.f),
        sf::Vector2f(-GameConst::ShipSize.width / 2.f, GameConst::ShipSize.height / 2.f),
        sf::Vector2f(GameConst::ShipSize.width / 2.f, -GameConst::ShipSize.height / 2.f),
        sf::Vector2f(GameConst::ShipSize.width / 2.f, GameConst::ShipSize.height / 2.f),
    };
}

LocalEliminationState::LocalEliminationState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_backgroundScene   (ctx.appInstance.getMessageBus()),
    m_gameScene         (ctx.appInstance.getMessageBus(), 1024),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_uiSounds          (m_audioResource),
    m_mapParser         (m_gameScene),
    m_renderPath        (m_resources),
    m_playerInputs      ({ sd.localPlayers[0].inputBinding,sd.localPlayers[1].inputBinding,sd.localPlayers[2].inputBinding,sd.localPlayers[3].inputBinding }),
    m_state             (Readying),
    m_suddenDeath       (false)
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
bool LocalEliminationState::handleEvent(const sf::Event& evt)
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

    for (auto& input : m_playerInputs)
    {
        input.handleEvent(evt);
    }
    m_backgroundScene.forwardEvent(evt);
    m_gameScene.forwardEvent(evt);
    m_uiScene.forwardEvent(evt);

    return true;
}

void LocalEliminationState::handleMessage(const xy::Message& msg)
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
        else if (data.type == GameEvent::RaceEnded)
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Game::Vehicle;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Disabled);
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
                auto rotation = vehicle.currentWaypoint.getComponent<WayPoint>().rotation;
                Transform t;
                t.setRotation(rotation);
                auto offset = SpawnOffsets[m_sharedData.localPlayers[vehicle.colourID].position];
                offset = t.transformPoint(offset.x, offset.y);

                tx.setPosition(vehicle.currentWaypoint.getComponent<xy::Transform>().getPosition() + offset);
                tx.setRotation(rotation);
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

            //m_gameScene.getActiveCamera().getComponent<CameraTarget>().lockedOn = false;
        }
        else if (data.type == VehicleEvent::LapLine)
        {
            auto id = data.entity.getComponent<Vehicle>().colourID;
            
            //count laps and end time trial when done
            if (m_sharedData.localPlayers[id].lapCount)
            {
                m_sharedData.localPlayers[id].lapCount--;
            }

            xy::Command cmd;
            cmd.targetFlags = CommandID::UI::LapText;
            cmd.action = [&, id](xy::Entity e, float)
            {
                e.getComponent<Nixie>().lowerValue = m_sharedData.localPlayers[id].lapCount;
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            //we'll go to sudden death mode instead
            if (m_sharedData.localPlayers[id].lapCount == 0)
            {                
                auto* msg = getContext().appInstance.getMessageBus().post<GameEvent>(MessageID::GameMessage);
                msg->type = GameEvent::SuddenDeath;

                m_suddenDeath = true;
            }
        }
    }

    m_backgroundScene.forwardMessage(msg);
    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool LocalEliminationState::update(float dt)
{
    auto camera = m_gameScene.getActiveCamera();
    auto camPosition = camera.getComponent<xy::Transform>().getPosition();

    //TODO state specific stuff probably wants moving to director
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

            cmd.targetFlags = CommandID::Game::LapLine;
            cmd.action = [](xy::Entity e, float)
            {
                //a bit kludgy but it makes the lights turn green! :P
                auto& verts = e.getComponent<xy::Drawable>().getVertices();
                for (auto i = 8u; i < 16u; ++i)
                {
                    verts[i].texCoords.x -= 192.f;
                }
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

            m_state = Racing;
        }
        break;
    case Racing:
    {
        //sort the play positions and tell the camera to target leader
        std::sort(m_playerEntities.begin(), m_playerEntities.end(),
            [](xy::Entity a, xy::Entity b)
            {
                return a.getComponent<Vehicle>().totalDistance + a.getComponent<Vehicle>().lapDistance >
                    b.getComponent<Vehicle>().totalDistance + b.getComponent<Vehicle>().lapDistance;
            });
        m_gameScene.getActiveCamera().getComponent<CameraTarget>().target = m_playerEntities[0];

        m_sharedData.localPlayers[m_playerEntities[0].getComponent<Vehicle>().colourID].position = 0;

        //check non-leading vehicles and eliminate if out of view
        sf::FloatRect viewRect(camPosition - (xy::DefaultSceneSize / 2.f), xy::DefaultSceneSize);
        
        auto count = 0;
        for (auto i = 1; i < 4; ++i)
        {
            //update race position
            m_sharedData.localPlayers[m_playerEntities[i].getComponent<Vehicle>().colourID].position = i;

            if (m_playerEntities[i].getComponent<Vehicle>().stateFlags == (1 << Vehicle::Eliminated))
            {
                count++;
            }
            else
            {
                if (!viewRect.contains(m_playerEntities[i].getComponent<xy::Transform>().getPosition())
                    && m_playerEntities[i].getComponent<Vehicle>().invincibleTime < 0)
                {
                    eliminate(m_playerEntities[i]);
                    count++;
                }
            }
        }

        //if eliminated count == 3 switch to celebration state and award player point
        if (count == 3
            && m_playerEntities[0].getComponent<Vehicle>().stateFlags == (1 << Vehicle::Normal))
        {
            m_celebrationTimer.restart();
            m_playerEntities[0].getComponent<Vehicle>().stateFlags = (1 << Vehicle::Celebrating);
            m_state = Celebrating;

            auto entity = m_playerEntities[0];
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

            //award point to player
            if (!m_suddenDeath)
            {
                auto playerID = m_playerEntities[0].getComponent<Vehicle>().colourID;
                m_sharedData.localPlayers[playerID].points++;

                auto* msg = getContext().appInstance.getMessageBus().post<GameEvent>(MessageID::GameMessage);
                msg->type = GameEvent::PlayerScored;
                msg->playerID = playerID;
                msg->score = m_sharedData.localPlayers[playerID].points;
            }
        }
    }
        break;
    case Celebrating:
        //count celebration time
        if (m_celebrationTimer.getElapsedTime() > CelebrationTime)
        {
            //on time out check scores - if player won, end the game
            //either 4 points or if sudden death is active
            if (m_sharedData.localPlayers[m_playerEntities[0].getComponent<Vehicle>().colourID].points == 4
                || m_suddenDeath)
            {
                m_sharedData.localPlayers[m_playerEntities[0].getComponent<Vehicle>().colourID].points = 4; //forces this player to win if sudden death
                requestStackPush(StateID::Summary);
                break;
            }

            //else respawn all at leader's respawn point and return to race state
            const auto& leaderVehicle = m_playerEntities[0].getComponent<Vehicle>();
            auto wp = leaderVehicle.currentWaypoint;
            auto lapDistance = leaderVehicle.lapDistance;
            auto lapCount = m_sharedData.localPlayers[leaderVehicle.colourID].lapCount;

            for (auto& player : m_sharedData.localPlayers)
            {
                player.lapCount = lapCount;
            }

            for (auto e : m_playerEntities)
            {
                auto& vehicle = e.getComponent<Vehicle>();

                vehicle.currentWaypoint = wp;
                vehicle.lapDistance = lapDistance;
                vehicle.waypointDistance = wp.getComponent<WayPoint>().trackDistance;
                vehicle.totalDistance = vehicle.waypointDistance;
                
                auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
                msg->type = VehicleEvent::RequestRespawn;
                msg->entity = e;
            }

            //TODO if the camera moves a long way (it shouldn't) it might eliminate players
            //as they respawn?? Tried to mitigate this by not eliminating players who have an invincibility time
            m_state = Racing;
        }
        break;
    }

    static float shaderTime = 0.f;
    shaderTime += dt;
    m_shaders.get(ShaderID::Globe).setUniform("u_time", shaderTime / 100.f);
    m_shaders.get(ShaderID::Asteroid).setUniform("u_time", -shaderTime / 10.f);

    for (auto& input : m_playerInputs)
    {
        input.update(dt);
    }

    m_backgroundScene.update(dt);
    m_gameScene.update(dt);
    m_uiScene.update(dt);

    
    m_backgroundScene.getActiveCamera().getComponent<xy::Transform>().setPosition(camPosition);
    m_renderPath.updateView(camPosition);

    return true;
}

void LocalEliminationState::draw()
{
    m_renderPath.render(m_backgroundScene, m_gameScene);

    auto& rw = getContext().renderWindow;
    rw.setView(getContext().defaultView);
    rw.draw(m_renderPath);

    rw.draw(m_uiScene);
}

//private
void LocalEliminationState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_backgroundScene.addSystem<xy::SpriteSystem>(mb);
    m_backgroundScene.addSystem<Sprite3DSystem>(mb);
    m_backgroundScene.addSystem<xy::CameraSystem>(mb);
    m_backgroundScene.addSystem<Camera3DSystem>(mb);
    m_backgroundScene.addSystem<xy::RenderSystem>(mb);
    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setView(xy::DefaultSceneSize);
    m_backgroundScene.getActiveCamera().getComponent<xy::Camera>().setViewport({ 0.f, 0.f, 1.f, 1.f });

    m_gameScene.addSystem<AIDriverSystem>(mb);
    m_gameScene.addSystem<VehicleSystem>(mb);
    m_gameScene.addSystem<AsteroidSystem>(mb);
    m_gameScene.addSystem<LightningSystem>(mb);
    m_gameScene.addSystem<InverseRotationSystem>(mb);
    m_gameScene.addSystem<TrailSystem>(mb);
    m_gameScene.addSystem<SkidEffectSystem>(mb);
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

    m_gameScene.addDirector<VFXDirector>(m_sprites);
    m_gameScene.addDirector<EliminationDirector>();

    m_uiScene.addSystem<xy::CommandSystem>(mb);
    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<NixieSystem>(mb);
    m_uiScene.addSystem<LapDotSystem>(mb);
    m_uiScene.addSystem<EliminationPointSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::SpriteAnimator>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);
    m_uiScene.addSystem<xy::AudioSystem>(mb);

    auto view = getContext().defaultView;
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setView(view.getSize());
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setViewport(view.getViewport());
}

void LocalEliminationState::loadResources()
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
    m_shaders.preload(ShaderID::Lightbar, LightbarFragment, sf::Shader::Fragment);

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

bool LocalEliminationState::loadMap()
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

    m_mapParser.addProps(m_matrixPool, m_shaders, m_resources, m_textureIDs);

    createRoids();

    return true;
}

void LocalEliminationState::createRoids()
{
    sf::FloatRect bounds = { sf::Vector2f(), m_mapParser.getSize() };
    bounds.left -= 100.f;
    bounds.top -= 100.f;
    bounds.width += 200.f;
    bounds.height += 200.f;
    m_gameScene.getSystem<AsteroidSystem>().setMapSize(bounds);

    m_gameScene.getSystem<AsteroidSystem>().setSpawnPosition(m_mapParser.getStartPosition().first);

    auto cameraEntity = m_gameScene.getActiveCamera();

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
        entity.addComponent<Sprite3D>(m_matrixPool).depth = radius * scale;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        auto shadowEnt = m_gameScene.createEntity();
        shadowEnt.addComponent<xy::Transform>().setPosition(sf::Vector2f(-18.f, 18.f));
        shadowEnt.addComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth - 1);
        shadowEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::RoidShadow]));
        entity.getComponent<xy::Transform>().addChild(shadowEnt.getComponent<xy::Transform>());
    }
}

void LocalEliminationState::buildUI()
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

    //lapline
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y - GameConst::LapLineOffset);
    entity.addComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapProgress]));
    bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    float scale = xy::DefaultSceneSize.x / bounds.width;
    entity.getComponent<xy::Transform>().setScale(scale, 1.f);

    //elimination points
    sf::Vector2f pointSize(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapPoint]).getSize());
    std::array<sf::Vector2f, 4u> positions =
    {
        sf::Vector2f(20.f, 120.f),
        sf::Vector2f(xy::DefaultSceneSize.x - (20.f + pointSize.x), 120.f),
        sf::Vector2f(20.f, 880.f),
        sf::Vector2f(xy::DefaultSceneSize.x - (20.f + pointSize.x), 880.f)
    };

    for (auto i = 0u; i < 4u; ++i)
    {
        entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(positions[i]);
        entity.addComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapPoint]));
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Lightbar));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.addComponent<EliminationDot>().ID = static_cast<std::uint8_t>(i);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::EliminationDot;
    }
}

void LocalEliminationState::addLapPoint(xy::Entity vehicle, sf::Color colour)
{
    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, xy::DefaultSceneSize.y - (GameConst::LapLineOffset - 8.f));
    entity.addComponent<xy::Drawable>().setDepth(1); //TODO each one needs own depth?
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(m_textureIDs[TextureID::Game::LapPoint])).setColour(colour);
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    entity.addComponent<LapDot>().parent = vehicle;
    entity.getComponent<LapDot>().trackLength = m_mapParser.getTrackLength();
}

void LocalEliminationState::spawnVehicle()
{
    auto [position, rotation] = m_mapParser.getStartPosition();

    xy::EmitterSettings smokeSettings;
    smokeSettings.loadFromFile("assets/particles/skidpuff.xyp", m_resources);

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

        spawnTrail(entity, GameConst::PlayerColour::Light[i]);

        m_sharedData.localPlayers[i].lapCount = m_sharedData.gameData.lapCount;

        if (m_sharedData.localPlayers[i].cpu)
        {
            entity.addComponent<AIDriver>().target = m_mapParser.getStartPosition().first;
            entity.getComponent<AIDriver>().skill = static_cast<AIDriver::Skill>(xy::Util::Random::value(AIDriver::Excellent, AIDriver::Bad));
        }
        else
        {
            m_playerInputs[i].setPlayerEntity(entity);
        }
        
        m_playerEntities[i] = entity;

        addLapPoint(entity, GameConst::PlayerColour::Light[i]);
    }

    auto cameraEntity = m_gameScene.getActiveCamera();
    cameraEntity.getComponent<CameraTarget>().target = m_playerEntities[0];
    cameraEntity.getComponent<xy::Transform>().setPosition(m_mapParser.getStartPosition().first);
}

void LocalEliminationState::spawnTrail(xy::Entity parent, sf::Color colour)
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

void LocalEliminationState::eliminate(xy::Entity entity)
{
    auto& vehicle = entity.getComponent<Vehicle>();
    vehicle.stateFlags = (1 << Vehicle::Eliminated);
    vehicle.accelerationMultiplier = 0.f;

    entity.getComponent<xy::Transform>().setScale(0.f, 0.f);

    auto* msg = getContext().appInstance.getMessageBus().post<VehicleEvent>(MessageID::VehicleMessage);
    msg->type = VehicleEvent::Eliminated;
    msg->entity = entity;

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

void LocalEliminationState::updateLoadingScreen(float dt, sf::RenderWindow& rw)
{
    m_sharedData.loadingScreen.update(dt);
    rw.draw(m_sharedData.loadingScreen);
}