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
#include "Camera3D.hpp"
#include "Sprite3D.hpp"
#include "AsteroidSystem.hpp"
#include "LightningSystem.hpp"

#include <glm/gtc/matrix_transform.hpp>

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
#include <xyginext/graphics/postprocess/Bloom.hpp>
#include <xyginext/util/Random.hpp>

namespace
{
#include "Sprite3DShader.inl"
#include "TrackShader.inl"
#include "GlobeShader.inl"
#include "BlurShader.inl"
#include "NeonBlendShader.inl"
#include "VehicleShader.inl"

    xy::Entity debugEnt;

    const sf::Time pingTime = sf::seconds(3.f);
}

RaceState::RaceState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_backgroundScene   (ctx.appInstance.getMessageBus()),
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

    //buildTest();

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
    m_backgroundScene.forwardEvent(evt);
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

    m_backgroundScene.forwardMessage(msg);
    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool RaceState::update(float dt)
{
    handlePackets();

    static float shaderTime = 0.f;
    shaderTime += dt;
    m_shaders.get(ShaderID::Globe).setUniform("u_time", shaderTime / 100.f);
    m_shaders.get(ShaderID::Asteroid).setUniform("u_time", -shaderTime / 10.f);

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
    m_backgroundScene.update(dt);
    m_gameScene.update(dt);
    m_uiScene.update(dt);

    auto& camPosition = m_gameScene.getActiveCamera().getComponent<xy::Transform>().getPosition();
    m_backgroundScene.getActiveCamera().getComponent<xy::Transform>().setPosition(camPosition);
    
    auto view = m_backgroundBuffer.getDefaultView();
    view.setCenter(camPosition);
    m_normalBuffer.setView(view);

    //m_shaders.get(ShaderID::Vehicle).setUniform("u_camWorldPosition", sf::Vector3f(xy::DefaultSceneSize.x / 2.f, xy::DefaultSceneSize.y / 2.f, Camera3D::depth));

    return true;
}

void RaceState::draw()
{
    //draw the background first for distort effect
    m_backgroundBuffer.setView(m_backgroundBuffer.getDefaultView());
    m_backgroundBuffer.clear();
    m_backgroundBuffer.draw(m_backgroundSprite);
    m_backgroundBuffer.draw(m_backgroundScene);
    m_backgroundBuffer.display();

    //normal map for distortion of background
    m_normalBuffer.clear({ 127,127,255 });
    m_normalBuffer.draw(m_normalSprite);
    m_normalBuffer.display();

    //draws the full scene including distorted background
    m_gameSceneBuffer.clear();
    m_gameSceneBuffer.draw(m_gameScene);
    m_gameSceneBuffer.display();

    //extract the brightest points to the neon buffer
    auto& extractShader = m_shaders.get(ShaderID::NeonExtract);
    //extractShader.setUniform("u_texture", m_gameSceneBuffer.getTexture());
    m_neonSprite.setTexture(m_gameSceneBuffer.getTexture(), true);
    m_neonBuffer.setView(m_gameSceneBuffer.getDefaultView());
    m_neonBuffer.clear(sf::Color::Transparent);
    m_neonBuffer.draw(m_neonSprite, &extractShader);
    m_neonBuffer.display();

    //blur passs
    static const float BlurAmount = 0.5f;
    auto& blurShader = m_shaders.get(ShaderID::Blur);
    blurShader.setUniform("u_texture", m_neonBuffer.getTexture());
    blurShader.setUniform("u_offset", sf::Vector2f(1.f / GameConst::SmallBufferSize.x, 0.f)/* * BlurAmount*/);
    m_neonSprite.setTexture(m_neonBuffer.getTexture(), true);
    m_blurBuffer.clear(sf::Color::Transparent);
    m_blurBuffer.draw(m_neonSprite, &blurShader);
    m_blurBuffer.display();

    blurShader.setUniform("u_texture", m_blurBuffer.getTexture());
    blurShader.setUniform("u_offset", sf::Vector2f(0.f, 1.f / GameConst::SmallBufferSize.y)/* * BlurAmount*/);
    m_neonSprite.setTexture(m_blurBuffer.getTexture(), true);
    m_neonBuffer.setView(m_neonBuffer.getDefaultView());
    m_neonBuffer.clear(sf::Color::Transparent);
    m_neonBuffer.draw(m_neonSprite, &blurShader);
    m_neonBuffer.display();

    //render blended output to window
    auto& blendShader = m_shaders.get(ShaderID::NeonBlend);
    //blendShader.setUniform("u_sceneTexture", m_gameSceneBuffer.getTexture());
    //blendShader.setUniform("u_neonTexture", m_neonBuffer.getTexture());

    auto& rw = getContext().renderWindow;
    rw.setView(getContext().defaultView);
    rw.draw(m_gameSceneSprite, &blendShader);
    rw.draw(m_uiScene);
}

//private
void RaceState::initScene()
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
    m_gameScene.addSystem<InterpolationSystem>(mb);
    m_gameScene.addSystem<DeadReckoningSystem>(mb);
    m_gameScene.addSystem<AsteroidSystem>(mb);
    m_gameScene.addSystem<LightningSystem>(mb);
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
    m_gameScene.addSystem<xy::AudioSystem>(mb);
    
    m_gameScene.addDirector<VFXDirector>(m_sprites);
    //m_gameScene.addPostProcess<xy::PostBloom>();

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
    TextureID::handles[TextureID::Stars] = m_resources.load<sf::Texture>("assets/images/stars.png");
    m_backgroundSprite.setTexture(m_resources.get<sf::Texture>(TextureID::handles[TextureID::Stars]), true);
    TextureID::handles[TextureID::StarsFar] = m_resources.load<sf::Texture>("assets/images/stars_far.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::StarsFar]).setRepeated(true);
    TextureID::handles[TextureID::StarsMid] = m_resources.load<sf::Texture>("assets/images/stars_mid.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::StarsMid]).setRepeated(true);
    TextureID::handles[TextureID::StarsNear] = m_resources.load<sf::Texture>("assets/images/stars_near.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::StarsNear]).setRepeated(true);

    TextureID::handles[TextureID::PlanetDiffuse] = m_resources.load<sf::Texture>("assets/images/globe_diffuse.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::PlanetDiffuse]).setRepeated(true);
    TextureID::handles[TextureID::RoidDiffuse] = m_resources.load<sf::Texture>("assets/images/roid_diffuse.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::RoidDiffuse]).setRepeated(true);
    TextureID::handles[TextureID::PlanetNormal] = m_resources.load<sf::Texture>("assets/images/crater_normal.png");
    m_resources.get<sf::Texture>(TextureID::handles[TextureID::PlanetNormal]).setRepeated(true);
    TextureID::handles[TextureID::RoidShadow] = m_resources.load<sf::Texture>("assets/images/roid_shadow.png");
    TextureID::handles[TextureID::VehicleNormal] = m_resources.load<sf::Texture>("assets/images/vehicles/vehicles_normal.png");

    if (!m_backgroundBuffer.create(GameConst::LargeBufferSize.x, GameConst::LargeBufferSize.y))
    {
        m_sharedData.errorMessage = "Failed to create background buffer";
        requestStackPush(StateID::Error);
        return;
    }

    if (!m_normalBuffer.create(GameConst::SmallBufferSize.x, GameConst::SmallBufferSize.y))
    {
        m_sharedData.errorMessage = "Failed to create normal buffer";
        requestStackPush(StateID::Error);
        return;
    }
    else
    {
        m_normalBuffer.setSmooth(true);
    }
    
    if (!m_neonBuffer.create(GameConst::SmallBufferSize.x, GameConst::SmallBufferSize.y))
    {
        m_sharedData.errorMessage = "Failed to create neon buffer";
        requestStackPush(StateID::Error);
        return;
    }
    else
    {
        m_neonBuffer.setSmooth(true);
    }

    if (!m_blurBuffer.create(GameConst::SmallBufferSize.x, GameConst::SmallBufferSize.y))
    {
        m_sharedData.errorMessage = "Failed to create blur buffer";
        requestStackPush(StateID::Error);
        return;
    }

    if (!m_gameSceneBuffer.create(GameConst::LargeBufferSize.x, GameConst::LargeBufferSize.y))
    {
        xy::Logger::log("Failed creating game scen buffer", xy::Logger::Type::Error);
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
    m_shaders.preload(ShaderID::Blur, BlurFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::NeonBlend, BlendFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::NeonExtract, ExtractFragment, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::Asteroid, SpriteVertex, GlobeFragment);
    m_shaders.preload(ShaderID::Vehicle, VehicleVertex, VehicleFrag);

    //only set these once if we can help it - no access to uniform IDs
    //in SFML means lots of string look-ups setting uniforms :(
    auto& blendShader = m_shaders.get(ShaderID::NeonBlend);
    blendShader.setUniform("u_sceneTexture", m_gameSceneBuffer.getTexture());
    blendShader.setUniform("u_neonTexture", m_neonBuffer.getTexture());

    auto& extractShader = m_shaders.get(ShaderID::NeonExtract);
    extractShader.setUniform("u_texture", m_gameSceneBuffer.getTexture());

    auto& vehicleShader = m_shaders.get(ShaderID::Vehicle);
    vehicleShader.setUniform("u_normalMap", m_resources.get<sf::Texture>(TextureID::handles[TextureID::VehicleNormal]));

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

    sf::FloatRect bounds(sf::Vector2f(), m_mapParser.getSize());
    bounds.left -= 1000.f;
    bounds.top -= 1000.f;
    bounds.width += 2000.f;
    bounds.height += 2000.f;
    m_gameScene.getSystem<AsteroidSystem>().setMapSize(bounds);
    m_gameScene.getSystem<AsteroidSystem>().setSpawnPosition(m_mapParser.getStartPosition());

    m_mapParser.renderLayers(m_trackTextures);
    m_normalSprite.setTexture(m_trackTextures[GameConst::TrackLayer::Normal].getTexture(), true);
    m_neonSprite.setTexture(m_trackTextures[GameConst::TrackLayer::Neon].getTexture(), true);
    m_gameSceneSprite.setTexture(m_gameSceneBuffer.getTexture(), true);

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(GameConst::TrackRenderDepth);
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.addComponent<xy::Sprite>(m_trackTextures[GameConst::TrackLayer::Track].getTexture());

    //add a camera
    auto view = getContext().defaultView;
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
    backgroundEnt.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::TrackDistortion));
    backgroundEnt.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_normalBuffer.getTexture());
    backgroundEnt.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    backgroundEnt.addComponent<xy::Sprite>(m_backgroundBuffer.getTexture());
    camEnt.getComponent<xy::Transform>().addChild(backgroundEnt.getComponent<xy::Transform>());

    /*auto neonEnt = m_gameScene.createEntity();
    neonEnt.addComponent<xy::Transform>().setOrigin(sf::Vector2f(GameConst::MediumBufferSize) / 2.f);
    neonEnt.getComponent<xy::Transform>().setScale(4.f, 4.f);
    neonEnt.addComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth + 1);
    neonEnt.getComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    neonEnt.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    neonEnt.addComponent<xy::Sprite>(m_neonBuffer.getTexture());
    camEnt.getComponent<xy::Transform>().addChild(neonEnt.getComponent<xy::Transform>());*/

    float fov = Camera3D::calcFOV(view.getSize().y);
    float ratio = view.getSize().x / view.getSize().y;
    camEnt.addComponent<Camera3D>().projectionMatrix = glm::perspective(fov, ratio, 10.f, Camera3D::depth + 2600.f);

    m_gameScene.setActiveCamera(camEnt);
    m_gameScene.setActiveListener(camEnt);

    //dem starrs
    std::array<std::size_t, 3u> IDs =
    {
        TextureID::handles[TextureID::StarsNear],
        TextureID::handles[TextureID::StarsMid],
        TextureID::handles[TextureID::StarsFar],
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
    entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.get<sf::Texture>(TextureID::handles[TextureID::PlanetNormal]));
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::PlanetDiffuse]));

    addProps();

    //let the server know the world is loaded so it can send us all player cars
    m_sharedData.netClient->sendPacket(PacketID::ClientMapLoaded, std::uint8_t(0), xy::NetFlag::Reliable);
}

void RaceState::addProps()
{
    const auto& barriers = m_mapParser.getBarriers();

    for (const auto& b : barriers)
    {
        auto entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>(); //points are in world space. Should we change this to improve culling?
        entity.addComponent<xy::Drawable>().setFilterFlags(GameConst::FilterFlags::All);
        entity.addComponent<Lightning>() = b;
    }
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

void RaceState::buildTest()
{
    auto temp = m_resources.load<sf::Texture>("assets/images/temp01a.png");
    
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setDepth(1000);
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(temp));

    auto cameraEntity = m_gameScene.getActiveCamera();
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Sprite>().getTexture());
    entity.addComponent<Sprite3D>(m_matrixPool).depth = 200.f;
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
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
                m_sharedData.racePositions = packet.as<std::array<std::uint64_t, 4u>>();
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
                m_sharedData.playerInfo.erase(ident.peerID);
                LOG("Removed player at peer " + std::to_string(ident.peerID), xy::Logger::Type::Info);
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
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Vehicle));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_diffuseMap");
    //entity.getComponent<xy::Drawable>().bindUniform("u_modelMatrix", entity.getComponent<xy::Transform>().getTransform().getMatrix());
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Car + data.vehicleType];
    entity.addComponent<Vehicle>().type = static_cast<Vehicle::Type>(data.vehicleType);
    //TODO we should probably get this from the server, but it might not matter
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

    m_playerInput.setPlayerEntity(entity);

    //temp for collision debugging
    /*entity.addComponent<xy::Callback>().active = true;
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
    };*/

    auto cameraEntity = m_gameScene.getActiveCamera();
    cameraEntity.getComponent<CameraTarget>().target = entity;

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
    entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::All);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::NetActor;
    entity.addComponent<CollisionObject>().type = CollisionObject::Type::NetActor;

    switch (data.actorID)
    {
    default:
        //TODO add default sprite
        break;
    case ActorID::Car:
    {
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Car];
        entity.getComponent<xy::Transform>().setOrigin(GameConst::CarSize.width * GameConst::VehicleCentreOffset, GameConst::CarSize.height / 2.f);       
        entity.getComponent<CollisionObject>().applyVertices(GameConst::CarPoints);       
        entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::CarSize);

        goto Vehicle;
    }
        break;
    case ActorID::Bike:
    {        
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Bike];
        entity.getComponent<xy::Transform>().setOrigin(GameConst::BikeSize.width * GameConst::VehicleCentreOffset, GameConst::BikeSize.height / 2.f);
        entity.getComponent<CollisionObject>().applyVertices(GameConst::BikePoints);       
        entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::BikeSize);

        goto Vehicle;
    }
        break;
    case ActorID::Ship:
    {
        entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Game::Ship];
        entity.getComponent<xy::Transform>().setOrigin(GameConst::ShipSize.width * GameConst::VehicleCentreOffset, GameConst::ShipSize.height / 2.f);
        entity.getComponent<CollisionObject>().applyVertices(GameConst::ShipPoints);
        entity.addComponent<xy::BroadphaseComponent>().setArea(GameConst::ShipSize);

        goto Vehicle;
    }
        break;

    Vehicle:
        entity.getComponent<xy::Drawable>().setDepth(GameConst::VehicleRenderDepth);
        entity.getComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Vehicle);
        entity.addComponent<xy::Callback>().function = ScaleCallback();

        break;

    case ActorID::Roid:
    {
        entity.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::RoidDiffuse]));
        auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
        auto radius = bounds.width / 2.f;
        entity.getComponent<xy::Transform>().setOrigin(radius, radius);
        entity.getComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth);
        entity.getComponent<xy::Drawable>().setFilterFlags(GameConst::Normal);
        entity.addComponent<Asteroid>().setRadius(radius * data.scale);
        
        entity.getComponent<CollisionObject>().applyVertices(createCollisionCircle(radius * 0.9f, { radius, radius }));
        entity.addComponent<xy::BroadphaseComponent>().setFilterFlags(CollisionFlags::Asteroid);
        entity.getComponent<xy::BroadphaseComponent>().setArea(bounds);

        auto cameraEntity = m_gameScene.getActiveCamera();
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Asteroid));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().bindUniform("u_normalMap", m_resources.get<sf::Texture>(TextureID::handles[TextureID::PlanetNormal]));
        entity.addComponent<Sprite3D>(m_matrixPool).depth = radius * data.scale;
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        //TODO I'd quite like to mask these with the track alpha channel but.. meh. Maybe later.
        auto shadowEnt = m_gameScene.createEntity();
        shadowEnt.addComponent<xy::Transform>().setPosition(sf::Vector2f(-18.f, 18.f));
        shadowEnt.addComponent<xy::Drawable>().setDepth(GameConst::RoidRenderDepth - 1);
        shadowEnt.addComponent<xy::Sprite>(m_resources.get<sf::Texture>(TextureID::handles[TextureID::RoidShadow]));
        entity.getComponent<xy::Transform>().addChild(shadowEnt.getComponent<xy::Transform>());
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