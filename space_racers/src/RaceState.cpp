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
#include "ServerStates.hpp"
#include "NetConsts.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>

RaceState::RaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_sharedData(sd),
    m_gameScene (ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildWorld();

    //temp while we get this set up...
    //eventually this will be done when the host creates a lobby
    if (!sd.server)
    {
        sd.server = std::make_unique<sv::Server>();
        sd.server->run(sv::StateID::Race);
    }

    sf::Clock tempClock;
    while (tempClock.getElapsedTime().asSeconds() < 2.f) {} //give the server time to start

    sd.netClient->connect("127.0.0.1", NetConst::Port);

    quitLoadingScreen();
}

//public
bool RaceState::handleEvent(const sf::Event& evt)
{
    for (auto& ip : m_playerInputs)
    {
        ip.handleEvent(evt);
    }

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
    for (auto& ip : m_playerInputs)
    {
        ip.update();
    }

    //poll event afterwards so gathered inputs are sent immediately
    xy::NetEvent evt;
    while (m_sharedData.netClient->pollEvent(evt))
    {

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


}

void RaceState::addLocalPlayers()
{
    auto tempID = m_resources.load<sf::Texture>("dummy resource");
    sf::FloatRect tempRect(0.f, 0.f, 135.f, 77.f);

    auto view = getContext().defaultView;

    for (auto i = 0u; i < m_sharedData.localPlayerCount; ++i)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().setOrigin(tempRect.width * Vehicle::centreOffset, tempRect.height / 2.f);
        entity.addComponent<Vehicle>().settings = Definition::car;
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID)).setTextureRect(tempRect);

        //update view as appropriate
        entity.addComponent<xy::Camera>().setView(view.getSize());
        entity.getComponent<xy::Camera>().setViewport(view.getViewport());
        entity.getComponent<xy::Camera>().lockRotation(true);
        m_gameScene.setActiveCamera(entity);

        m_playerInputs.emplace_back(entity, m_sharedData.inputBindings[i]);
    }
}