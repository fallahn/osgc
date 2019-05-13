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

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>

RaceState::RaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_gameScene     (ctx.appInstance.getMessageBus()),
    m_playerInput   (sd.inputBindings[0], *sd.netClient)
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
    m_playerInput.update();

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
    entity.addComponent<xy::Drawable>().setDepth(-20);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID));

    //let the server know the world is loaded so it can send us all player cars
    m_sharedData.netClient->sendPacket(PacketID::ClientMapLoaded, std::uint8_t(0), xy::NetFlag::Reliable);
}

void RaceState::spawnVehicle(const VehicleData& data)
{
    //TODO spawn vehicle
    std::cout << "Spawned vehicle!\n";
    //TODO check if data has our peer ID and add to controller

    //TODO count spawned vehicles and tell server when all are spawned

    //TODO map vehicle entities to server ID for easy updating from incoming packets
}