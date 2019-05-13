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

#include "DebugState.hpp"
#include "VehicleSystem.hpp"

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <fstream>

namespace
{
    struct VehicleTest final
    {
        //test values for imgui output
        float angularDrag = 0.8f;
        float turnSpeed = 0.717f; //rads per second

        float drag = 0.877f; //multiplier
        float acceleration = 84.f; //units per second

        float maxSpeed = 1010.f;
    };
    std::array<VehicleTest, 3> vehicles = { };
    std::array<sf::FloatRect, 3> sizes = {sf::FloatRect(0.f, 0.f, 135.f, 77.f), sf::FloatRect(0.f, 0.f, 132.f, 40.f), sf::FloatRect(0.f, 0.f, 132.f, 120.f)};
    std::int32_t vehicleIndex = 0;

    std::uint32_t vehicleCommandID = 1;

    void saveData()
    {
        std::ofstream file(xy::FileSystem::getResourcePath() + "assets/vehicle.dat", std::ios::binary);
        if (file.is_open() && file.good())
        {
            file.write((char*)vehicles.data(), sizeof(VehicleTest) * vehicles.size());
        }
    }

    void writeHeader(const std::string& path)
    {
        std::ofstream file(path);
        if (file.is_open() && file.good())
        {
            const std::array<std::string, 3u> names =
            {
                "car", "bike", "ship"
            };

            file << "#pragma once\n\n";
            file << "#include \"VehicleSystem.hpp\"\n\n";

            file << "namespace Definition\n{\n";
            for (auto i = 0u; i < names.size(); ++i)
            {
                file << "    static const Vehicle::Settings " << names[i] 
                    << "("<<vehicles[i].angularDrag<<"f,"<<vehicles[i].turnSpeed<<"f,"<<vehicles[i].drag<<"f,"<<vehicles[i].acceleration<<"f);\n";
            }
            file << "}";
        }
    }
}

DebugState::DebugState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State(ss, ctx),
    m_sharedData(sd),
    m_gameScene(ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildWorld();
    addLocalPlayers();


    registerWindow(
        [&]()
        {
            xy::Nim::begin("Vehicle Properties");
            xy::Nim::slider("Drag", vehicles[vehicleIndex].drag, 0.f, 0.995f);
            xy::Nim::slider("MaxSpeed", vehicles[vehicleIndex].maxSpeed, 400.f, 1200.f);
            vehicles[vehicleIndex].acceleration = vehicles[vehicleIndex].maxSpeed / (vehicles[vehicleIndex].drag / (1.f - vehicles[vehicleIndex].drag));
            xy::Nim::text("Acc. " + std::to_string(vehicles[vehicleIndex].acceleration));

            xy::Nim::separator();
            xy::Nim::slider("Angular Drag", vehicles[vehicleIndex].angularDrag, 0.f, 0.995f);
            xy::Nim::slider("Turn Speed", vehicles[vehicleIndex].turnSpeed, 0.1f, 1.5f);

            auto oldIndex = vehicleIndex;
            xy::Nim::simpleCombo("Vehicle", vehicleIndex, "Car\0Bike\0Ship\0\0");

            if (oldIndex != vehicleIndex)
            {
                xy::Command cmd;
                cmd.targetFlags = vehicleCommandID;
                cmd.action = [](xy::Entity e, float)
                {
                    e.getComponent<xy::Sprite>().setTextureRect(sizes[vehicleIndex]);
                    e.getComponent<xy::Transform>().setOrigin(sizes[vehicleIndex].width * Vehicle::centreOffset, sizes[vehicleIndex].height / 2.f);
                };
                m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }

            if (xy::Nim::button("Save"))
            {
                saveData();
            }

            xy::Nim::sameLine();

            if (xy::Nim::button("Generate"))
            {
                auto output = xy::FileSystem::saveFileDialogue(xy::FileSystem::getResourcePath(), "hpp");
                if (!output.empty())
                {
                    if (xy::FileSystem::getFileExtension(output) != ".hpp")
                    {
                        output += ".hpp";
                    }
                    writeHeader(output);
                }
            }

            xy::Nim::end();
        });

    std::ifstream file(xy::FileSystem::getResourcePath() + "assets/vehicle.dat", std::ios::binary);
    if (file.is_open() && file.good())
    {
        //TODO check we have the expected file size
        file.read((char*)vehicles.data(), sizeof(VehicleTest) * vehicles.size());
    }

    quitLoadingScreen();
}

DebugState::~DebugState()
{
    saveData();
}

//public
bool DebugState::handleEvent(const sf::Event& evt)
{
    for (auto& ip : m_playerInputs)
    {
        ip.handleEvent(evt);
    }

    m_gameScene.forwardEvent(evt);
    return true;
}

void DebugState::handleMessage(const xy::Message& msg)
{
    //TODO handle resize message and update all cameras

    m_gameScene.forwardMessage(msg);
}

bool DebugState::update(float dt)
{
    for (auto& ip : m_playerInputs)
    {
        ip.update();
    }

    xy::Command cmd;
    cmd.targetFlags = vehicleCommandID;
    cmd.action = [](xy::Entity e, float)
    {
        auto& v = e.getComponent<Vehicle>();
        v.settings.acceleration = vehicles[vehicleIndex].acceleration;
        v.settings.angularDrag = vehicles[vehicleIndex].angularDrag;
        v.settings.drag = vehicles[vehicleIndex].drag;
        v.settings.turnSpeed = vehicles[vehicleIndex].turnSpeed;
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    m_gameScene.update(dt);
    return true;
}

void DebugState::draw()
{
    auto& rw = getContext().renderWindow;

    rw.draw(m_gameScene);
}

//private
void DebugState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<VehicleSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
}

void DebugState::loadResources()
{

}

void DebugState::buildWorld()
{
    auto temp = m_resources.load<sf::Texture>("assets/images/temp01.png");

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-20);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(temp));

}

void DebugState::addLocalPlayers()
{
    auto tempID = m_resources.load<sf::Texture>("dummy resource");
    sf::FloatRect tempRect(0.f, 0.f, 135.f, 77.f);

    auto view = getContext().defaultView;

    for (auto i = 0u; i < m_sharedData.localPlayerCount; ++i)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
        entity.getComponent<xy::Transform>().setOrigin(tempRect.width * Vehicle::centreOffset, tempRect.height / 2.f);
        entity.addComponent<Vehicle>();
        entity.addComponent<xy::Drawable>();
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID)).setTextureRect(tempRect);
        entity.addComponent<xy::CommandTarget>().ID = vehicleCommandID;

        //update view as appropriate
        auto camEnt = m_gameScene.createEntity();
        camEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
        camEnt.addComponent<xy::Camera>().setView(view.getSize());
        camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());
        camEnt.getComponent<xy::Camera>().lockRotation(true);
        m_gameScene.setActiveCamera(camEnt);

        entity.getComponent<xy::Transform>().addChild(camEnt.getComponent<xy::Transform>());

        m_playerInputs.emplace_back(entity, m_sharedData.inputBindings[i]);
    }
}