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
#include "InterpolationSystem.hpp"
#include "DeadReckoningSystem.hpp"
#include "CommandIDs.hpp"
#include "CameraTarget.hpp"
#include "MessageIDs.hpp"
#include "AnimationCallbacks.hpp"
#include "VFXDirector.hpp"

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

namespace
{
    xy::Entity debugEnt;

    const sf::Time pingTime = sf::seconds(3.f);
}

RaceState::RaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_gameScene         (ctx.appInstance.getMessageBus()),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_uiSounds          (m_audioResource),
    m_mapParser         (m_gameScene),
    m_playerInput       (sd.inputBindings[0], sd.netClient.get())
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildWorld();
    buildUI();

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
        }
    }

    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool RaceState::update(float dt)
{
    handlePackets();

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
    m_gameScene.update(dt);
    m_uiScene.update(dt);
    return true;
}

void RaceState::draw()
{
    auto& rw = getContext().renderWindow;

    rw.draw(m_gameScene);
    rw.draw(m_uiScene);
}

//private
void RaceState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<VehicleSystem>(mb);
    m_gameScene.addSystem<InterpolationSystem>(mb);
    m_gameScene.addSystem<DeadReckoningSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<CameraTargetSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
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
    TextureID::handles[TextureID::Temp01] = m_resources.load<sf::Texture>("nothing_to_see_here");
    TextureID::handles[TextureID::Temp02] = m_resources.load<sf::Texture>("assets/images/temp02.png");

    xy::SpriteSheet spriteSheet;

    //game scene assets
    spriteSheet.loadFromFile("assets/sprites/dust_puff.spt", m_resources);
    m_sprites[SpriteID::Game::SmokePuff] = spriteSheet.getSprite("dust_puff");

    spriteSheet.loadFromFile("assets/sprites/explosion.spt", m_resources);
    m_sprites[SpriteID::Game::Explosion] = spriteSheet.getSprite("explosion");

    //ui scene assets
    spriteSheet.loadFromFile("assets/sprites/lights.spt", m_resources);
    m_sprites[SpriteID::Game::UIStartLights] = spriteSheet.getSprite("lights");

    m_uiSounds.loadFromFile("assets/sound/ui.xas");


}

void RaceState::buildWorld()
{
    if (!m_mapParser.load("assets/maps/AceOfSpace.tmx"))
    {
        m_sharedData.errorMessage = "Client was unable to load map";
        requestStackClear();
        requestStackPush(StateID::Error);
        m_sharedData.netClient->disconnect();
        return;
    }

    auto tempID = m_resources.load<sf::Texture>("assets/images/temp01.png");
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID));

    //let the server know the world is loaded so it can send us all player cars
    m_sharedData.netClient->sendPacket(PacketID::ClientMapLoaded, std::uint8_t(0), xy::NetFlag::Reliable);
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
            case PacketID::RaceTimerStarted:
                showTimer();
                break;
            case PacketID::RaceFinished:
                m_playerInput.getPlayerEntity().getComponent<Vehicle>().stateFlags = (1 << Vehicle::Disabled);
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
            case PacketID::DebugPosition:
                if (debugEnt.isValid())
                {
                    debugEnt.getComponent<xy::Transform>().setPosition(packet.as<sf::Vector2f>());
                }
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
    entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp01]));
    entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(data.vehicleType);
    //TODO we shopuld probably get this from the server, but it might not matter
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
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::CarSize); //TODO remove this
        entity.getComponent<CollisionObject>().applyVertices(GameConst::CarPoints);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);
        break;
    case Vehicle::Bike:
        entity.getComponent<Vehicle>().settings = Definition::bike;
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::BikeSize);
        entity.getComponent<CollisionObject>().applyVertices(GameConst::BikePoints);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::BikeSize);
        break;
    case Vehicle::Ship:
        entity.getComponent<Vehicle>().settings = Definition::ship;
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::ShipSize);
        entity.getComponent<CollisionObject>().applyVertices(GameConst::ShipPoints);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::ShipSize);
        break;
    }
    auto bounds = entity.getComponent<xy::BroadphaseComponent>().getArea();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width * GameConst::VehicleCentreOffset, bounds.height / 2.f);

    m_playerInput.setPlayerEntity(entity);

    //temp for collision debugging
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [](xy::Entity e, float)
    {
        if (e.getComponent<Vehicle>().collisionFlags)
        {
            e.getComponent<xy::Sprite>().setColour(sf::Color::Blue);
        }
        else
        {
            e.getComponent<xy::Sprite>().setColour(sf::Color::White);
        }
    };

    //add a camera
    auto view = getContext().defaultView;
    auto camEnt = m_gameScene.createEntity();
    camEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
    camEnt.addComponent<xy::Camera>().setView(view.getSize());
    camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());
    camEnt.getComponent<xy::Camera>().lockRotation(true);
    camEnt.addComponent<CameraTarget>().target = entity;
    camEnt.addComponent<xy::AudioListener>();
    
    m_gameScene.setActiveCamera(camEnt);
    m_gameScene.setActiveListener(camEnt);

#ifdef XY_DEBUG
    /*debugEnt = m_gameScene.createEntity();
    debugEnt.addComponent<xy::Transform>().setOrigin(10.f, 10.f);
    debugEnt.addComponent<xy::Drawable>().setDepth(100);
    debugEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp01])).setTextureRect({ -10.f, -10.f, 20.f, 20.f });
    debugEnt.getComponent<xy::Sprite>().setColour(sf::Color::Blue);*/
#endif //XY_DEBUG

    //count spawned vehicles and tell server when all are spawned
    m_sharedData.gameData.actorCount--;
}

void RaceState::spawnActor(const ActorData& data)
{
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(data.x, data.y);
    entity.getComponent<xy::Transform>().setRotation(data.rotation);
    entity.getComponent<xy::Transform>().setScale(data.scale, data.scale);
    entity.addComponent<InterpolationComponent>();
    entity.addComponent<DeadReckon>().prevTimestamp = data.timestamp; //important, initial reckoning will be way off otherwise
    entity.addComponent<NetActor>().actorID = data.actorID;
    entity.getComponent<NetActor>().serverID = data.serverID;
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::NetActor;
    entity.addComponent<CollisionObject>().type = CollisionObject::Type::NetActor;

    switch (data.actorID)
    {
    default:
        //TODO add default sprite
        break;
    case ActorID::Car:
    {
        entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp01]));
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::CarSize);
        entity.getComponent<xy::Sprite>().setColour(sf::Color(127,0,0,220));
        entity.getComponent<xy::Transform>().setOrigin(GameConst::CarSize.width * GameConst::VehicleCentreOffset, GameConst::CarSize.height / 2.f);
        
        entity.getComponent<CollisionObject>().applyVertices(GameConst::CarPoints);
        entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);
        entity.addComponent<xy::Callback>().function = ScaleCallback();
    }
        break;
    case ActorID::Bike:
    {
        entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp01]));
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::BikeSize);
        entity.getComponent<xy::Transform>().setOrigin(GameConst::BikeSize.width * GameConst::VehicleCentreOffset, GameConst::BikeSize.height / 2.f);

        entity.getComponent<CollisionObject>().applyVertices(GameConst::BikePoints);
        entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::BikeSize);
        entity.addComponent<xy::Callback>().function = ScaleCallback();
    }
        break;
    case ActorID::Ship:
    {
        entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp01]));
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::ShipSize);
        entity.getComponent<xy::Transform>().setOrigin(GameConst::ShipSize.width * GameConst::VehicleCentreOffset, GameConst::ShipSize.height / 2.f);

        entity.getComponent<CollisionObject>().applyVertices(GameConst::ShipPoints);
        entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
        entity.getComponent<xy::BroadphaseComponent>().setArea(GameConst::ShipSize);
        entity.addComponent<xy::Callback>().function = ScaleCallback();
    }
        break;
    case ActorID::Roid:
    {
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp02]));
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        auto radius = bounds.width / 2.f;
        entity.getComponent<xy::Transform>().setOrigin(radius, radius);
        entity.getComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth);

        
        entity.getComponent<CollisionObject>().applyVertices(createCollisionCircle(radius * 0.9f, { radius, radius }));
        entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Asteroid);
        entity.getComponent<xy::BroadphaseComponent>().setArea(bounds);
    }
        break;
    }

    //count actors so we can tell the server when we're ready
    m_sharedData.gameData.actorCount--;
}

void RaceState::updateActor(const ActorUpdate& update)
{
    xy::Command cmd;
    cmd.targetFlags = CommandID::NetActor;
    cmd.action = [update](xy::Entity e, float)
    {
        auto& actor = e.getComponent<NetActor>();
        if (actor.serverID == update.serverID)
        {
            //we still use interpolation for rotation
            InterpolationPoint point;
            //point.position = { update.x, update.y };
            point.rotation = update.rotation;
            point.timestamp = update.timestamp;
            e.getComponent<InterpolationComponent>().setTarget(point);

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
                e.getComponent<xy::Callback>().active = false;
                e.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
                e.getComponent<xy::Transform>().setScale(1.f, 1.f);
                e.getComponent<xy::Transform>().setPosition(data.x, data.y);

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
                e.getComponent<xy::Transform>().setScale(0.f, 0.f);

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
    xy::Command cmd;
    cmd.targetFlags = CommandID::NetActor;
    cmd.action = [id](xy::Entity e, float)
    {
        if (e.getComponent<NetActor>().serverID == id)
        {
            e.getComponent<xy::Callback>().active = true; //performs scaling
            e.getComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth - 2);
        }
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
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
    auto& font = m_sharedData.resources.get<sf::Font>(FontID::handles[FontID::Default]);

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