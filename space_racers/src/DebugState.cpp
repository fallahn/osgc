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
#include "GameConsts.hpp"
#include "AsteroidSystem.hpp"
#include "CollisionObject.hpp"
#include "ShapeUtils.hpp"
#include "CameraTarget.hpp"
#include "MessageIDs.hpp"
#include "VehicleDefs.hpp"
#include "WayPoint.hpp"

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

#include <xyginext/gui/Gui.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Vector.hpp>

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

        float accelStr = 2.f;

        float maxSpeed = 1010.f;
    };
    std::array<VehicleTest, 3> vehicles = { };
    std::array<sf::FloatRect, 3> sizes = {GameConst::CarSize, GameConst::BikeSize, GameConst::ShipSize};
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
    : xy::State     (ss, ctx),
    m_sharedData    (sd),
    m_gameScene     (ctx.appInstance.getMessageBus()),
    m_playerInput   (sd.localPlayers[0].inputBinding, nullptr),
    m_otherInput    (sd.localPlayers[1].inputBinding, nullptr),
    m_mapParser     (m_gameScene)
{
    launchLoadingScreen();

    initScene();
    loadResources();
    buildWorld();
    addLocalPlayers();

    ctx.appInstance.setMouseCursorVisible(true);

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
            xy::Nim::slider("Accel Strength", vehicles[vehicleIndex].accelStr, 1.f, 3.f);

            auto oldIndex = vehicleIndex;
            xy::Nim::simpleCombo("Vehicle", vehicleIndex, "Car\0Bike\0Ship\0\0");

            if (oldIndex != vehicleIndex)
            {
                xy::Command cmd;
                cmd.targetFlags = vehicleCommandID;
                cmd.action = [](xy::Entity e, float)
                {
                    e.getComponent<xy::Sprite>().setTextureRect(sizes[vehicleIndex]);
                    e.getComponent<xy::Transform>().setOrigin(sizes[vehicleIndex].width * GameConst::VehicleCentreOffset, sizes[vehicleIndex].height / 2.f);
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
                /*auto output = xy::FileSystem::saveFileDialogue(xy::FileSystem::getResourcePath(), "hpp");
                if (!output.empty())
                {
                    if (xy::FileSystem::getFileExtension(output) != ".hpp")
                    {
                        output += ".hpp";
                    }
                    writeHeader(output);
                }*/
                xy::Logger::log("I broke this don't use it!!");
            }

            xy::Nim::sameLine();

            if (xy::Nim::button("quit"))
            {
                requestStackClear();
                requestStackPush(StateID::MainMenu);
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
    return true;
}

void DebugState::handleMessage(const xy::Message& msg)
{
    if (msg.id == MessageID::VehicleMessage)
    {
        const auto& data = msg.getData<VehicleEvent>();
        if (data.type == VehicleEvent::RequestRespawn)
        {
            auto entity = data.entity;
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
            
            entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        }
        else if (data.type == VehicleEvent::Fell)
        {
            m_gameScene.getActiveCamera().getComponent<CameraTarget>().lockedOn = false;

            auto entity = data.entity;
            entity.getComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth - 1);
        }
    }

    m_gameScene.forwardMessage(msg);
}

bool DebugState::update(float dt)
{
    m_playerInput.update(dt);
    m_otherInput.update(dt);

    xy::Command cmd;
    cmd.targetFlags = vehicleCommandID;
    cmd.action = [](xy::Entity e, float)
    {
        auto& v = e.getComponent<Vehicle>();
        v.settings.acceleration = vehicles[vehicleIndex].acceleration;
        v.settings.angularDrag = vehicles[vehicleIndex].angularDrag;
        v.settings.drag = vehicles[vehicleIndex].drag;
        v.settings.turnSpeed = vehicles[vehicleIndex].turnSpeed;
        v.settings.accelStrength = vehicles[vehicleIndex].accelStr;
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
    m_gameScene.addSystem<AsteroidSystem>(mb);
    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<CameraTargetSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<xy::RenderSystem>(mb);
}

void DebugState::loadResources()
{

}

void DebugState::buildWorld()
{
    //track texture
    auto temp = m_resources.load<sf::Texture>("assets/images/temp01.png");

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(-20);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(temp));

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    bounds.left -= 100.f;
    bounds.top -= 100.f;
    bounds.width += 200.f;
    bounds.height += 200.f;
    m_gameScene.getSystem<AsteroidSystem>().setMapSize(bounds);

    //load map
    if (!m_mapParser.load("assets/maps/AceOfSpace.tmx"))
    {
        m_sharedData.errorMessage = "Failed to load map";
        requestStackPush(StateID::Error);
        return;
    }
    
    m_gameScene.getSystem<AsteroidSystem>().setSpawnPosition(m_mapParser.getStartPosition().first);

    //asteroids
    temp = m_resources.load<sf::Texture>("assets/images/temp02.png");

    auto positions = xy::Util::Random::poissonDiscDistribution(bounds, 1200, 8);
    for (auto position : positions)
    {
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position);
        entity.addComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth);
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(temp));

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

        auto debugEnt = m_gameScene.createEntity();
        debugEnt.addComponent<xy::Transform>();
        auto& verts = debugEnt.addComponent<xy::Drawable>().getVertices();
        for (auto p : collisionVerts)
        {
            verts.emplace_back(p, sf::Color::Red);
        }
        verts.emplace_back(collisionVerts.front(), sf::Color::Red);
        debugEnt.getComponent<xy::Drawable>().updateLocalBounds();
        debugEnt.getComponent<xy::Drawable>().setDepth(100);
        debugEnt.getComponent<xy::Drawable>().setPrimitiveType(sf::LineStrip);
        entity.getComponent<xy::Transform>().addChild(debugEnt.getComponent<xy::Transform>());
    }

}

void DebugState::addLocalPlayers()
{
    auto tempID = m_resources.load<sf::Texture>("dummy resource");

    auto view = getContext().defaultView;

    auto entity = m_gameScene.createEntity();
    auto [position, rotation] = m_mapParser.getStartPosition();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setRotation(rotation);
    sf::Transform offsetTransform;
    offsetTransform.rotate(rotation);
    auto offset = offsetTransform.transformPoint(GameConst::SpawnPositions[0]);
    entity.getComponent<xy::Transform>().move(offset);
    entity.getComponent<xy::Transform>().setOrigin(GameConst::CarSize.width * GameConst::VehicleCentreOffset, GameConst::CarSize.height / 2.f);
    entity.addComponent<Vehicle>().waypointCount = m_mapParser.getWaypointCount();
    entity.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Normal);
    entity.addComponent<CollisionObject>().type = CollisionObject::Vehicle;
    entity.getComponent<CollisionObject>().applyVertices(GameConst::CarPoints);
    entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID)).setTextureRect(GameConst::CarSize);
    entity.addComponent<xy::CommandTarget>().ID = vehicleCommandID;

    //callback to set sprite colour on collision
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
        [](xy::Entity e, float)
    {
        e.getComponent<xy::Sprite>().setColour(sf::Color::White);
        if (e.getComponent<Vehicle>().collisionFlags)
        {
            e.getComponent<xy::Sprite>().setColour(sf::Color::Blue);
        }
    };

    //collision debug
    auto cEnt = m_gameScene.createEntity();
    cEnt.addComponent<xy::Transform>();
    Shape::setPolyLine(cEnt.addComponent<xy::Drawable>(), entity.getComponent<CollisionObject>().vertices);
    cEnt.getComponent<xy::Drawable>().setDepth(100);
    cEnt.getComponent<xy::Drawable>().updateLocalBounds();
    entity.getComponent<xy::Transform>().addChild(cEnt.getComponent<xy::Transform>());

    //update view as appropriate
    auto camEnt = m_gameScene.createEntity();
    camEnt.addComponent<xy::Transform>().setPosition(entity.getComponent<xy::Transform>().getOrigin());
    camEnt.addComponent<xy::Camera>().setView(view.getSize());
    camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());
    camEnt.getComponent<xy::Camera>().lockRotation(true);
    camEnt.addComponent<CameraTarget>().target = entity;
    m_gameScene.setActiveCamera(camEnt);

    m_playerInput.setPlayerEntity(entity);


    //another car for collision testing
    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.getComponent<xy::Transform>().setRotation(rotation);

    offset = offsetTransform.transformPoint(GameConst::SpawnPositions[1]);
    entity.getComponent<xy::Transform>().move(offset);
    entity.getComponent<xy::Transform>().setOrigin(GameConst::CarSize.width * GameConst::VehicleCentreOffset, GameConst::CarSize.height / 2.f);
    entity.addComponent<Vehicle>().waypointCount = m_mapParser.getWaypointCount();
    entity.getComponent<Vehicle>().settings = Definition::car;
    entity.getComponent<Vehicle>().stateFlags = (1 << Vehicle::Normal);
    entity.addComponent<CollisionObject>().type = CollisionObject::Vehicle;
    entity.getComponent<CollisionObject>().applyVertices(GameConst::CarPoints);
    entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);
    entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(tempID)).setTextureRect(GameConst::CarSize);
    m_otherInput.setPlayerEntity(entity); //vehicle system won't update vehicle physics without an input parser
}