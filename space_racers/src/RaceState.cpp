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
#include "ResourceIDs.hpp"
#include "CommandIDs.hpp"
#include "CameraTarget.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

namespace
{
    xy::Entity debugEnt;
}

RaceState::RaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_gameScene         (ctx.appInstance.getMessageBus()),
    m_mapParser         (m_gameScene),
    m_playerInput       (sd.inputBindings[0], sd.netClient.get())
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildWorld();

    quitLoadingScreen();
}

//public
bool RaceState::handleEvent(const sf::Event& evt)
{
    m_playerInput.handleEvent(evt);
    m_gameScene.forwardEvent(evt);
    return true;
}

void RaceState::handleMessage(const xy::Message& msg)
{
    //TODO handle resize message and update all cameras

    m_gameScene.forwardMessage(msg);
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

    m_playerInput.update(dt);
    m_gameScene.update(dt);
    return true;
}

void RaceState::draw()
{
    auto& rw = getContext().renderWindow;

    rw.draw(m_gameScene);
}

//private
void RaceState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<VehicleSystem>(mb);
    m_gameScene.addSystem<InterpolationSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<CameraTargetSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
}

void RaceState::loadResources()
{
    TextureID::handles[TextureID::Temp01] = m_resources.load<sf::Texture>("nothing_to_see_here");
    TextureID::handles[TextureID::Temp02] = m_resources.load<sf::Texture>("assets/images/temp02.png");
}

void RaceState::buildWorld()
{
    m_mapParser.load("assets/maps/AceOfSpace.tmx");

    auto tempID = m_resources.load<sf::Texture>("assets/images/temp01.png");
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID));

    //let the server know the world is loaded so it can send us all player cars
    m_sharedData.netClient->sendPacket(PacketID::ClientMapLoaded, std::uint8_t(0), xy::NetFlag::Reliable);
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
    //camEnt.g

    m_gameScene.setActiveCamera(camEnt);

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
    entity.addComponent<NetActor>().actorID = data.actorID;
    entity.getComponent<NetActor>().serverID = data.serverID;
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::CommandTarget>().ID = CommandID::NetActor;

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
        entity.getComponent<xy::Transform>().setOrigin(GameConst::CarSize.width * GameConst::VehicleCentreOffset, GameConst::CarSize.height / 2.f);
    }
        break;
    case ActorID::Bike:
    {
        entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp01]));
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::BikeSize);
        entity.getComponent<xy::Transform>().setOrigin(GameConst::BikeSize.width * GameConst::VehicleCentreOffset, GameConst::BikeSize.height / 2.f);
    }
        break;
    case ActorID::Ship:
    {
        entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp01]));
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::ShipSize);
        entity.getComponent<xy::Transform>().setOrigin(GameConst::ShipSize.width * GameConst::VehicleCentreOffset, GameConst::ShipSize.height / 2.f);
    }
        break;
    case ActorID::Roid:
    {
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Temp02]));
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        entity.getComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth);

        float radius = data.scale * bounds.width;
        entity.addComponent<CollisionObject>().type = CollisionObject::Type::Roid;
        entity.getComponent<CollisionObject>().applyVertices(createCollisionCircle(radius * 0.9f, { radius, radius }));
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
        if (e.getComponent<NetActor>().serverID == update.serverID)
        {
            InterpolationPoint point;
            point.position = { update.x, update.y };
            point.rotation = update.rotation;
            point.timestamp = update.timestamp;
            e.getComponent<InterpolationComponent>().setTarget(point);
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