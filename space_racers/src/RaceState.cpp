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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>

namespace
{
    xy::Entity debugEnt;
}

RaceState::RaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_gameScene     (ctx.appInstance.getMessageBus()),
    m_playerInput   (sd.inputBindings[0], sd.netClient.get())
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
    m_playerInput.update(dt);

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
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
}

void RaceState::loadResources()
{

}

void RaceState::buildWorld()
{
    auto tempID = m_resources.load<sf::Texture>("assets/images/temp01.png");
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID));

    //let the server know the world is loaded so it can send us all player cars
    m_sharedData.netClient->sendPacket(PacketID::ClientMapLoaded, std::uint8_t(0), xy::NetFlag::Reliable);
}

void RaceState::spawnVehicle(const VehicleData& data)
{
    auto tempID = m_resources.load<sf::Texture>("nothing_to_see_here");

    //spawn vehicle
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(data.x, data.y);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID));
    entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(data.vehicleType);

    switch (entity.getComponent<Vehicle>().type)
    {
    default:
    case Vehicle::Car:
        entity.getComponent<Vehicle>().settings = Definition::car;
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::CarSize); //TODO replace this with collision
        break;
    case Vehicle::Bike:
        entity.getComponent<Vehicle>().settings = Definition::bike;
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::BikeSize);
        break;
    case Vehicle::Ship:
        entity.getComponent<Vehicle>().settings = Definition::ship;
        entity.getComponent<xy::Sprite>().setTextureRect(GameConst::ShipSize);
        break;
    }
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width * Vehicle::centreOffset, bounds.height / 2.f);

    m_playerInput.setPlayerEntity(entity);

    //add a camera
    auto view = getContext().defaultView;
    auto camEnt = m_gameScene.createEntity();
    camEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
    camEnt.addComponent<xy::Camera>().setView(view.getSize());
    camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());
    camEnt.getComponent<xy::Camera>().lockRotation(true);
    entity.getComponent<xy::Transform>().addChild(camEnt.getComponent<xy::Transform>());

    m_gameScene.setActiveCamera(camEnt);

#ifdef XY_DEBUG
    debugEnt = m_gameScene.createEntity();
    debugEnt.addComponent<xy::Transform>();
    debugEnt.addComponent<xy::Drawable>().setDepth(100);
    debugEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID)).setTextureRect({ -10.f, -10.f, 20.f, 20.f });
    debugEnt.getComponent<xy::Sprite>().setColour(sf::Color::Blue);
#endif //XY_DEBUG

    //TODO count spawned vehicles and tell server when all are spawned
}

void RaceState::spawnActor(const ActorData& data)
{
    std::cout << "spawned actor " << data.actorID << "\n";
}

void RaceState::reconcile(const ClientUpdate& update)
{
    auto entity = m_playerInput.getPlayerEntity();
    if (entity.isValid())
    {
        m_gameScene.getSystem<VehicleSystem>().reconcile(update, entity);
    }
}