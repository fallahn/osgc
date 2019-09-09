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

#include "GameState.hpp"
#include "IslandSystem.hpp"
#include "Camera3D.hpp"
#include "Sprite3D.hpp"
#include "CommandIDs.hpp"
#include "GlobalConsts.hpp"
#include "MessageIDs.hpp"
#include "IslandGenerator.hpp"
#include "NetworkClient.hpp"
#include "SharedStateData.hpp"
#include "PlayerSystem.hpp"
#include "AnimationSystem.hpp"
#include "DayNightSystem.hpp"
#include "CollisionSystem.hpp"
#include "CollisionBounds.hpp"
#include "TorchlightSystem.hpp"
#include "CarriableSystem.hpp"
#include "InventorySystem.hpp"
#include "GameUI.hpp"
#include "ClientWeaponSystem.hpp"
#include "AnimationCallbacks.hpp"
#include "HealthBarSystem.hpp"
#include "DepthAnimationSystem.hpp"
#include "QuadTreeFilter.hpp"
#include "BoatSystem.hpp"
#include "BotSystem.hpp"
#include "WetPatchDirector.hpp"
#include "Operators.hpp"
#include "AudioSystem.hpp"
#include "SpringFlower.hpp"
#include "SeagullSystem.hpp"
#include "ParrotSystem.hpp"
#include "SoundEffectsDirector.hpp"
#include "ExplosionSystem.hpp"
#include "BroadcastSystem.hpp"
#include "RainSystem.hpp"
#include "FoliageSystem.hpp"
#include "ClientCarriableCallback.hpp"
#include "CompassSystem.hpp"
#include "FlappySailSystem.hpp"
#include "Render3DSystem.hpp"
#include "ClientBeeSystem.hpp"
#include "ConCommands.hpp"
#include "ShadowCastSystem.hpp"
#include "SimpleShadowSystem.hpp"
#include "ClientDecoySystem.hpp"
#include "ClientFlareSystem.hpp"
#include "InterpolationSystem.hpp"
#include "Packet.hpp"

#include <xyginext/core/App.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/AudioListener.hpp>
#include <xyginext/ecs/components/BroadPhaseComponent.hpp>

#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/ParticleSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>

#include <xyginext/graphics/SpriteSheet.hpp>
#include <xyginext/util/Vector.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/network/NetData.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Audio/Listener.hpp>

#include "glm/gtc/matrix_transform.hpp"

namespace
{
#include "SpriteShader.inl"
#include "IslandShaders.inl"
#include "ParticleShader.inl"
#include "SunsetShader.inl"
#include "GroundShaders.inl"
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_gameScene         (ctx.appInstance.getMessageBus(), 512),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_sharedData        (sd),
    m_inputParser       (sd.inputBinding, *sd.netClient),
    m_nameTagManager    (m_fontResource),
    m_summaryTexture    (m_fontResource),
    m_miniMap           (m_textureResource),
    m_foliageGenerator  (m_textureResource, m_shaderResource, m_modelMatrices),
    m_sceneLoaded       (false),
    m_audioScape        (m_audioResource),
    m_canShowMessage    (true),
    m_roundTime         (3 * 60),
    m_roundOver         (false),
    m_updateSummary     (false)
{
    xy::App::setMouseCursorVisible(false);

    launchLoadingScreen();
    loadResources();

    m_summaryStats.stats[0].id = Actor::ID::PlayerOne;
    m_summaryStats.stats[1].id = Actor::ID::PlayerTwo;
    m_summaryStats.stats[2].id = Actor::ID::PlayerThree;
    m_summaryStats.stats[3].id = Actor::ID::PlayerFour;

#ifdef XY_DEBUG
    //register some console commands
    registerCommand("set_storm", [&](const std::string& param)
    {
        Server::ConCommand::Data data;
        data.commandID = Server::ConCommand::Weather;

        if (param == "0")
        {
            data.value.asInt = 0;
        }
        else if (param == "1")
        {
            data.value.asInt = 1;
        }
        else if (param == "2")
        {
            data.value.asInt = 2;
        }
        else
        {
            return;
        }
        m_sharedData.netClient->sendPacket(PacketID::ConCommand, data, xy::NetFlag::Reliable);
    });

    registerCommand("bots_enabled", [&](const std::string& param)
    {
        Server::ConCommand::Data data;
        data.commandID = Server::ConCommand::BotEnable;
        data.value.asInt = (param == "0") ? 0 : 1;
        m_sharedData.netClient->sendPacket(PacketID::ConCommand, data, xy::NetFlag::Reliable);
    });

    registerCommand("spawn", [&](const std::string& param)
    {
        if (!param.empty())
        {
            Actor::ID id = Actor::None;
            if (param == "decoy_item")
            {
                id = Actor::DecoyItem;
            }
            else if (param == "decoy")
            {
                id = Actor::Decoy;
            }
            else if (param == "crab")
            {
                id = Actor::Crab;
            }
            else if (param == "flare_item")
            {
                id = Actor::FlareItem;
            }
            else if (param == "flare")
            {
                id = Actor::Flare;
            }
            else if (param == "skull_item")
            {
                id = Actor::SkullItem;
            }
            else if (param == "skull_shield")
            {
                id = Actor::SkullShield;
            }

            if (id != Actor::None)
            {
                Server::ConCommand::Data data;
                data.commandID = Server::ConCommand::Spawn;
                data.value.asInt = id;
                data.position = m_inputParser.getPlayerEntity().getComponent<xy::Transform>().getPosition();
                data.position.x += Global::TileSize;
                m_sharedData.netClient->sendPacket(PacketID::ConCommand, data, xy::NetFlag::Reliable);
            }
            else
            {
                xy::Console::print(param + ": Actor not found.");
            }
        }
    });
#endif

    quitLoadingScreen();   
}

//public
bool GameState::handleEvent(const sf::Event& evt)
{
    if (xy::ui::wantsMouse() || xy::ui::wantsKeyboard())
    {
        return true;
    }

    auto zoomMap = [&]()
    {
        xy::Command cmd;
        cmd.targetFlags = UI::CommandID::MiniMap;
        cmd.action = [](xy::Entity entity, float)
        {
            auto& mapData = entity.getComponent<MiniMapData>();
            if (mapData.targetScale == 0.5f)
            {
                mapData.targetScale = 2.f;
                mapData.targetPosition = Global::MapZoomPosition;
            }
            else
            {
                mapData.targetScale = 0.5f;
                mapData.targetPosition = Global::MapOnPosition;
            }
            entity.getComponent<xy::Callback>().active = true;
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        auto* msg = getContext().appInstance.getMessageBus().post<UIEvent>(MessageID::UIMessage);
        msg->action = UIEvent::MiniMapZoom;
    };

    auto showMap = [&]()
    {
        xy::Command cmd;
        cmd.targetFlags = UI::CommandID::MiniMap;
        cmd.action = [&](xy::Entity entity, float)
        {
            auto& mapData = entity.getComponent<MiniMapData>();
            if (mapData.targetPosition.x < 0)
            {
                mapData.targetScale = 0.5f;
                mapData.targetPosition = Global::MapOnPosition;

                auto* msg = getContext().appInstance.getMessageBus().post<UIEvent>(MessageID::UIMessage);
                msg->action = UIEvent::MiniMapShow;
            }
            else
            {
                mapData.targetScale = 0.5f;
                mapData.targetPosition = Global::MapOffPosition;

                auto* msg = getContext().appInstance.getMessageBus().post<UIEvent>(MessageID::UIMessage);
                msg->action = UIEvent::MiniMapHide;
            }
            entity.getComponent<xy::Callback>().active = true;
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    };

    auto hideScores = [&]()
    {
        xy::Command cmd;
        cmd.targetFlags = UI::CommandID::RoundEndMenu;
        cmd.action = [](xy::Entity entity, float)
        {
            entity.getComponent<xy::Transform>().setPosition(UI::RoundScreenOffPosition);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        /*cmd.targetFlags = UI::CommandID::TreasureScore;
        cmd.action = [](xy::Entity entity, float)
        {
            entity.getComponent<xy::Transform>().setPosition(UI::RoundScreenOffPosition + UI::TreasureScorePosition);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);*/
    };

    auto showScores = [&]()
    {
        xy::Command cmd;
        cmd.targetFlags = UI::CommandID::RoundEndMenu;
        cmd.action = [](xy::Entity entity, float)
        {
            entity.getComponent<xy::Transform>().setPosition(UI::RoundScreenOnPosition);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        /*cmd.targetFlags = UI::CommandID::TreasureScore;
        cmd.action = [](xy::Entity entity, float)
        {
            entity.getComponent<xy::Transform>().setPosition(UI::RoundScreenOnPosition + UI::TreasureScorePosition);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);*/
    };

    if (evt.type == sf::Event::KeyReleased)
    {
        if (evt.key.code == sf::Keyboard::Tab
            && !m_roundOver)
        {
            hideScores();
        }
        else if (evt.key.code == m_sharedData.inputBinding.keys[InputBinding::ZoomMap])
        {
            zoomMap();
        }
        else if (evt.key.code == m_sharedData.inputBinding.keys[InputBinding::ShowMap])
        {
            //showMap();
            zoomMap();
        }
        //else if (evt.key.code == sf::Keyboard::End) m_gameScene.getActiveCamera().getComponent<Camera3D>().shakeAmount = 1.f;
#ifdef XY_DEBUG
        else if (evt.key.code == sf::Keyboard::Escape)
        {
            xy::App::quit();
        }
#endif
    }
    else if (evt.type == sf::Event::JoystickButtonReleased)
    {
        if (evt.joystickButton.joystickId == m_sharedData.inputBinding.controllerID)
        {
            if (evt.joystickButton.button == m_sharedData.inputBinding.buttons[InputBinding::ZoomMap])
            {
                zoomMap();
            }
            else if (evt.joystickButton.button == m_sharedData.inputBinding.buttons[InputBinding::ShowMap])
            {
                //showMap();
                zoomMap();
            }
            else if (evt.joystickButton.button == 7 && !m_roundOver)
            {
                hideScores();
            }
        }
    }
    else if (evt.type == sf::Event::KeyPressed)
    {
        if (evt.key.code == sf::Keyboard::Tab
            && !m_roundOver)
        {
            showScores();
        }
    }
    else if (evt.type == sf::Event::JoystickButtonPressed)
    {
        if (evt.joystickButton.joystickId == m_sharedData.inputBinding.controllerID)
        {
            if (evt.joystickButton.button == 7 && !m_roundOver)
            {
                showScores();
            }
        }
    }

    m_inputParser.handleEvent(evt);

    m_gameScene.forwardEvent(evt);
    m_uiScene.forwardEvent(evt);

    m_uiScene.getSystem<xy::UISystem>().handleEvent(evt);

    return false;
}

void GameState::handleMessage(const xy::Message& msg)
{
    serverMessageHandler(m_sharedData, msg);

    if (msg.id == MessageID::MapMessage)
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::Loaded)
        {
            xy::Command cmd;
            cmd.targetFlags = UI::CommandID::Curtain;
            cmd.action = [](xy::Entity ent, float) {ent.getComponent<xy::Callback>().active = true; };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        else if (data.type == MapEvent::ItemInWater)
        {
            auto entity = m_gameScene.createEntity();
            entity.addComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::PlaneShader));
            entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Rings];
            entity.addComponent<xy::SpriteAnimation>().play(0);
            entity.addComponent<float>() = 0.4f;
            entity.addComponent<xy::Callback>().active = true;
            entity.getComponent<xy::Callback>().function = 
                [&](xy::Entity e, float dt)
            {
                e.getComponent<float>() -= dt;
                if (e.getComponent<float>() < 0)
                {
                    m_gameScene.destroyEntity(e);
                }
            };
            auto& ringTx = entity.addComponent<xy::Transform>();
            auto bounds = m_sprites[SpriteID::Rings].getTextureBounds();
            ringTx.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
            ringTx.setPosition(data.position);
        }
    }
    else if (msg.id == MessageID::SceneMessage)
    {
        const auto& data = msg.getData<SceneEvent>();
        if (data.type == SceneEvent::CameraLocked)
        {
            m_inputParser.setEnabled(true);
        }
        else if (data.type == SceneEvent::CameraUnlocked)
        {
            m_inputParser.setEnabled(false);
        }
        else if (data.type == SceneEvent::SlideFinished)
        {
            if (data.id == UI::CommandID::RoundEndMenu)
            {
                //show the OK button and remove player input
                m_inputParser.setEnabled(false);
                showEndButton();
            }
        }
        else if (data.type == SceneEvent::MessageDisplayFinished)
        {
            m_canShowMessage = true;
        }
    }
    else if (msg.id == MessageID::CarryMessage)
    {
        const auto& data = msg.getData<CarryEvent>();
        if (data.action == CarryEvent::PickedUp)
        {
            //send a UI message if it's treasure
            if (data.type == Carrier::Treasure)
            {
                auto playerID = data.entity.getComponent<Actor>().id;
                if (playerID != Actor::Skeleton)
                {
                    sf::String screenMsg;

                    xy::Command cmd;
                    switch (playerID)
                    {
                    default: break;
                    case Actor::ID::PlayerOne:
                        cmd.targetFlags = UI::CommandID::CarryIconOne;
                        screenMsg = m_sharedData.clientInformation.getClient(0).name;
                        break;
                    case Actor::ID::PlayerTwo:
                        cmd.targetFlags = UI::CommandID::CarryIconTwo;
                        screenMsg = m_sharedData.clientInformation.getClient(1).name;
                        break;
                    case Actor::ID::PlayerThree:
                        cmd.targetFlags = UI::CommandID::CarryIconThree;
                        screenMsg = m_sharedData.clientInformation.getClient(2).name;
                        break;
                    case Actor::ID::PlayerFour:
                        cmd.targetFlags = UI::CommandID::CarryIconFour;
                        screenMsg = m_sharedData.clientInformation.getClient(3).name;
                        break;
                    }
                    cmd.action = [](xy::Entity entity, float)
                    {
                        entity.getComponent<xy::Sprite>().setColour(sf::Color::White);
                    };
                    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                    screenMsg += " found something shiny!";
                    printMessage(screenMsg);
                }
            }
        }
        else
        {
            //we don't always know what we dropped
            //but if we were carrying *something* it's
            //safe to hide it on the UI
            xy::Command cmd;
            auto playerID = data.entity.getComponent<Actor>().id;
            switch (playerID)
            {
            default: break;
            case Actor::ID::PlayerOne:
                cmd.targetFlags = UI::CommandID::CarryIconOne;
                break;
            case Actor::ID::PlayerTwo:
                cmd.targetFlags = UI::CommandID::CarryIconTwo;
                break;
            case Actor::ID::PlayerThree:
                cmd.targetFlags = UI::CommandID::CarryIconThree;
                break;
            case Actor::ID::PlayerFour:
                cmd.targetFlags = UI::CommandID::CarryIconFour;
                break;
            }
            cmd.action = [](xy::Entity entity, float)
            {
                entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
    else if (msg.id == MessageID::PlayerMessage)
    {
        const auto& data = msg.getData<PlayerEvent>();
        if (data.action == PlayerEvent::Died
            || data.action == PlayerEvent::Respawned)
        {
            xy::Command cmd;
            switch (data.entity.getComponent<Actor>().id)
            {
            default: break;
            case Actor::ID::PlayerOne:
                cmd.targetFlags = UI::CommandID::DeadIconOne;
                break;
            case Actor::ID::PlayerTwo:
                cmd.targetFlags = UI::CommandID::DeadIconTwo;
                break;
            case Actor::ID::PlayerThree:
                cmd.targetFlags = UI::CommandID::DeadIconThree;
                break;
            case Actor::ID::PlayerFour:
                cmd.targetFlags = UI::CommandID::DeadIconFour;
                break;
            }

            if (data.action == PlayerEvent::Died)
            {
                cmd.action = [](xy::Entity entity, float)
                {
                    entity.getComponent<xy::Sprite>().setColour(sf::Color::White);
                };
            }
            else
            {
                cmd.action = [](xy::Entity entity, float)
                {
                    entity.getComponent<xy::Sprite>().setColour(sf::Color::Transparent);
                };
            }
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }

    m_miniMap.handleMessage(msg);
    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
#ifdef XY_DEBUG
    static std::size_t packetSize = 0;
    static std::size_t frames = 0;
    static float bw = 0.f;

    xy::NetEvent evt;
    while (m_sharedData.netClient->pollEvent(evt))
    {
        handlePacket(evt);
        packetSize += evt.packet.getSize() + 1;
    }

    frames++;
    if (frames == 60)
    {
        //this is only an estimate...
        bw = static_cast<float>(packetSize * 8) / 1000.f;
        packetSize = 0;
        frames = 0;
    }
    xy::Console::printStat("Incoming bw", std::to_string(bw) + "Kbps");
#else
    xy::NetEvent evt;
    while (m_sharedData.netClient->pollNetwork(evt))
    {
        handlePacket(evt);
    }
#endif //XY_DEBUG
    
    if (!m_sceneLoaded)
    {
        m_sharedData.netClient->sendPacket(PacketID::RequestMap, std::uint8_t(0), xy::NetFlag::Reliable);
    }
    
    m_inputParser.update(dt);
    m_gameScene.update(dt);
    m_uiScene.update(dt);

    m_miniMap.update();

    processMessageQueue(); //messages in corner of screen

    if (m_updateSummary)
    {
        m_summaryTexture.update(m_summaryStats, m_sharedData.clientInformation);
        m_updateSummary = false;
    }

    return false;
}

void GameState::draw()
{
    m_miniMap.updateTexture();

    auto& rw = *xy::App::getRenderWindow();
    rw.draw(m_gameScene);
    rw.draw(m_uiScene);
}

//private
void GameState::loadResources()
{
    m_shaderResource.preload(ShaderID::SpriteShader, "#version 120\n#define LIGHTING\n" + SpriteVertex, SpriteFrag);
    m_shaderResource.preload(ShaderID::SpriteShaderCulled, "#version 120\n#define CULL_FADE\n#define LIGHTING\n" + SpriteVertex, SpriteFrag);
    m_shaderResource.preload(ShaderID::SpriteShaderUnlit, "#version 120\n#define CULL_FADE\n" + SpriteVertex, "#version 120\n" + UnlitSpriteFrag);
    m_shaderResource.preload(ShaderID::SpriteShaderUnlitTextured, "#version 120\n#define CULL_FADE\n" + SpriteVertex, "#version 120\n#define TEXTURED\n" + UnlitSpriteFrag);
    m_shaderResource.preload(ShaderID::ShadowShader, GroundVert, ShadowFrag);

    m_shaderResource.preload(ShaderID::SeaShader, SeaFrag, sf::Shader::Fragment);
    m_shaderResource.preload(ShaderID::PlaneShader, GroundVertLit, SpriteFrag);
    m_shaderResource.preload(ShaderID::LandShader, GroundVertLit, GroundFrag);
    m_shaderResource.preload(ShaderID::SkyShader, SkyFrag, sf::Shader::Fragment);
    m_shaderResource.preload(ShaderID::MoonShader, MoonFrag2, sf::Shader::Fragment);
    m_shaderResource.preload(ShaderID::SunsetShader, Sunset::Fragment, sf::Shader::Fragment);

    //don't forget to add any new sprite shaders which require cam position
    //such as distant faded ones, to this list
    std::vector<sf::Shader*> spriteShaders = 
    {
        &m_shaderResource.get(ShaderID::SpriteShader),
        &m_shaderResource.get(ShaderID::SpriteShaderCulled),
        &m_shaderResource.get(ShaderID::SpriteShaderUnlit),
        &m_shaderResource.get(ShaderID::SpriteShaderUnlitTextured)
    };

    auto& mb = getContext().appInstance.getMessageBus();
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<BotSystem>(mb, m_pathFinder, m_sharedData.seedData.hash);
    m_gameScene.addSystem<PlayerSystem>(mb);
    m_gameScene.addSystem<DepthAnimationSystem>(mb);
    m_gameScene.addSystem<SpringFlowerSystem>(mb);
    m_gameScene.addSystem<SeagullSystem>(mb);
    m_gameScene.addSystem<ParrotSystem>(mb);
    m_gameScene.addSystem<HealthBarSystem>(mb); 
    m_gameScene.addSystem<CollisionSystem>(mb);
    m_gameScene.addSystem<InterpolationSystem>(mb);
    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<ClientBeeSystem>(mb);
    m_gameScene.addSystem<AnimationSystem>(mb);
    m_gameScene.addSystem<BroadcastSystem>(mb);
    m_gameScene.addSystem<ExplosionSystem>(mb);
    m_gameScene.addSystem<RainSystem>(mb);
    m_gameScene.addSystem<FoliageSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<Camera3DSystem>(mb);
    m_gameScene.addSystem<Sprite3DSystem>(mb, spriteShaders);
    const auto& dns = m_gameScene.addSystem<DayNightSystem>(mb, m_shaderResource, m_textureResource);
    m_gameScene.addSystem<TorchlightSystem>(mb, m_shaderResource);
    m_gameScene.addSystem<IslandSystem>(mb, m_textureResource, m_shaderResource).setReflectionTexture(dns.getBufferTexture());
    m_gameScene.addSystem<CompassSystem>(mb); //must be done after the 3D camera update because it relies on cam matrix
    m_gameScene.addSystem<ClientWeaponSystem>(mb); //must come after 3d sprite for sorting
    m_gameScene.addSystem<ShadowCastSystem>(mb, m_shaderResource);
    m_gameScene.addSystem<SimpleShadowSystem>(mb);
    m_gameScene.addSystem<ClientDecoySystem>(mb);
    m_gameScene.addSystem<ClientFlareSystem>(mb);
    m_gameScene.addSystem<FlappySailSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::TextSystem>(mb);
    m_gameScene.addSystem<AudioSystem>(mb);
    
    m_gameScene.addSystem<Render3DSystem>(mb);
  
    m_gameScene.getSystem<Camera3DSystem>().enableChase(true);
    m_gameScene.getSystem<Camera3DSystem>().setActive(false);

    m_gameScene.addDirector<SFXDirector>(m_audioResource);

    //set up camera
    auto view = getContext().defaultView;
    auto cameraEntity = m_gameScene.getActiveCamera();
    auto& camera = cameraEntity.getComponent<xy::Camera>();
    camera.setView(view.getSize());
    camera.setViewport(view.getViewport());

    float fov = 0.6f; //0.55f FOV
    float aspect = 16.f / 9.f;
    cameraEntity.addComponent<Camera3D>().projectionMatrix = glm::perspective(fov, aspect, 0.1f, 1536.f); 
    cameraEntity.getComponent<xy::Transform>().setPosition(Global::IslandSize / 2.f);
    cameraEntity.getComponent<xy::AudioListener>().setDepth(Camera3D::height);

    //actually requires fov in X not Y
    m_gameScene.getSystem<Render3DSystem>().setFOV(fov * aspect);

    //actor sprites
    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/players.spt", m_textureResource);
    m_sprites[SpriteID::PlayerOne] = spriteSheet.getSprite("player_one");
    m_animationMaps[SpriteID::PlayerOne] = 
    {
        spriteSheet.getAnimationIndex("idle_up", "player_one"),
        spriteSheet.getAnimationIndex("idle_down", "player_one"),
        spriteSheet.getAnimationIndex("idle_left", "player_one"),
        spriteSheet.getAnimationIndex("idle_right", "player_one"),
        spriteSheet.getAnimationIndex("walk_left", "player_one"),
        spriteSheet.getAnimationIndex("walk_right", "player_one"),
        spriteSheet.getAnimationIndex("walk_up", "player_one"),
        spriteSheet.getAnimationIndex("walk_down", "player_one"),
        0,0
    };
    m_sprites[SpriteID::PlayerTwo] = spriteSheet.getSprite("player_two");
    m_animationMaps[SpriteID::PlayerTwo] =
    {
        spriteSheet.getAnimationIndex("idle_up", "player_two"),
        spriteSheet.getAnimationIndex("idle_down", "player_two"),
        spriteSheet.getAnimationIndex("idle_left", "player_two"),
        spriteSheet.getAnimationIndex("idle_right", "player_two"),
        spriteSheet.getAnimationIndex("walk_left", "player_two"),
        spriteSheet.getAnimationIndex("walk_right", "player_two"),
        spriteSheet.getAnimationIndex("walk_up", "player_two"),
        spriteSheet.getAnimationIndex("walk_down", "player_two"),
        0,0
    };
    m_sprites[SpriteID::PlayerThree] = spriteSheet.getSprite("player_three");
    m_animationMaps[SpriteID::PlayerThree] =
    {
        spriteSheet.getAnimationIndex("idle_up", "player_three"),
        spriteSheet.getAnimationIndex("idle_down", "player_three"),
        spriteSheet.getAnimationIndex("idle_left", "player_three"),
        spriteSheet.getAnimationIndex("idle_right", "player_three"),
        spriteSheet.getAnimationIndex("walk_left", "player_three"),
        spriteSheet.getAnimationIndex("walk_right", "player_three"),
        spriteSheet.getAnimationIndex("walk_up", "player_three"),
        spriteSheet.getAnimationIndex("walk_down", "player_three"),
        0,0
    };
    m_sprites[SpriteID::PlayerFour] = spriteSheet.getSprite("player_four");
    m_animationMaps[SpriteID::PlayerFour] =
    {
        spriteSheet.getAnimationIndex("idle_up", "player_four"),
        spriteSheet.getAnimationIndex("idle_down", "player_four"),
        spriteSheet.getAnimationIndex("idle_left", "player_four"),
        spriteSheet.getAnimationIndex("idle_right", "player_four"),
        spriteSheet.getAnimationIndex("walk_left", "player_four"),
        spriteSheet.getAnimationIndex("walk_right", "player_four"),
        spriteSheet.getAnimationIndex("walk_up", "player_four"),
        spriteSheet.getAnimationIndex("walk_down", "player_four"),
        0,0
    };

    spriteSheet.loadFromFile("assets/sprites/barrel.spt", m_textureResource);
    m_sprites[SpriteID::BarrelOne] = spriteSheet.getSprite("barrel01");
    m_animationMaps[SpriteID::BarrelOne] =
    {
        spriteSheet.getAnimationIndex("idle", "barrel01"),
        spriteSheet.getAnimationIndex("break", "barrel01"),
        spriteSheet.getAnimationIndex("explode", "barrel01"),
        0,0,0,0,0,0,0
    };

    m_sprites[SpriteID::BarrelTwo] = spriteSheet.getSprite("barrel02");
    m_animationMaps[SpriteID::BarrelTwo] =
    {
        spriteSheet.getAnimationIndex("idle", "barrel02"),
        spriteSheet.getAnimationIndex("break", "barrel02"),
        spriteSheet.getAnimationIndex("explode", "barrel02"),
        0,0,0,0,0,0,0
    };

    spriteSheet.loadFromFile("assets/sprites/skeleton.spt", m_textureResource);
    
    m_sprites[SpriteID::Skeleton] = spriteSheet.getSprite("zombie");
    m_animationMaps[SpriteID::Skeleton] = 
    {
        spriteSheet.getAnimationIndex("idle_up", "zombie"),
        spriteSheet.getAnimationIndex("idle_down", "zombie"),
        spriteSheet.getAnimationIndex("idle_left", "zombie"),
        spriteSheet.getAnimationIndex("idle_right", "zombie"),
        spriteSheet.getAnimationIndex("walk_left", "zombie"),
        spriteSheet.getAnimationIndex("walk_right", "zombie"),
        spriteSheet.getAnimationIndex("walk_up", "zombie"),
        spriteSheet.getAnimationIndex("walk_down", "zombie"),
        spriteSheet.getAnimationIndex("spawn", "zombie"),
        spriteSheet.getAnimationIndex("die", "zombie")
    };

    spriteSheet.loadFromFile("assets/sprites/ghosts.spt", m_textureResource);
    m_sprites[SpriteID::Ghost] = spriteSheet.getSprite("ghost");

    spriteSheet.loadFromFile("assets/sprites/poopsnail.spt", m_textureResource);
    m_sprites[SpriteID::Crab] = spriteSheet.getSprite("poopsnail");
    m_animationMaps[SpriteID::Crab] =
    {
        0,0,0,0,
        spriteSheet.getAnimationIndex("walk", "poopsnail"),
        0,0,
        0,0,
        spriteSheet.getAnimationIndex("dig", "poopsnail")
    };

    spriteSheet.loadFromFile("assets/sprites/lantern.spt", m_textureResource);
    m_sprites[SpriteID::Lantern] = spriteSheet.getSprite("lantern");
    auto id = spriteSheet.getAnimationIndex("flicker", "lantern");
    m_animationMaps[SpriteID::Lantern] = { id };

    spriteSheet.loadFromFile("assets/sprites/chest.spt", m_textureResource);
    m_sprites[SpriteID::Treasure] = spriteSheet.getSprite("chest");
    auto front = spriteSheet.getAnimationIndex("down", "chest");
    auto side = spriteSheet.getAnimationIndex("left", "chest");
    auto back = spriteSheet.getAnimationIndex("up", "chest");
    m_animationMaps[SpriteID::Treasure] = 
    {
        back, front, side, side,
        side, side, back, front,
        0,0
    };

    spriteSheet.loadFromFile("assets/sprites/wet_patch.spt", m_textureResource);
    m_sprites[SpriteID::WetPatch] = spriteSheet.getSprite("wet_patch");

    spriteSheet.loadFromFile("assets/sprites/weapons.spt", m_textureResource);
    m_sprites[SpriteID::WeaponRodney] = spriteSheet.getSprite("weapon_rodney");
    m_sprites[SpriteID::WeaponJean] = spriteSheet.getSprite("weapon_jean");
    m_gameScene.getSystem<ClientWeaponSystem>().setAnimations(spriteSheet);

    spriteSheet.loadFromFile("assets/sprites/collectibles.spt", m_textureResource);
    m_sprites[SpriteID::Ammo] = spriteSheet.getSprite("ammo");
    m_sprites[SpriteID::Coin] = spriteSheet.getSprite("coin");
    m_sprites[SpriteID::Food] = spriteSheet.getSprite("food");

    spriteSheet.loadFromFile("assets/sprites/rings.spt", m_textureResource);
    m_sprites[SpriteID::Rings] = spriteSheet.getSprite("rings");

    spriteSheet.loadFromFile("assets/sprites/fire.spt", m_textureResource);
    m_sprites[SpriteID::Fire] = spriteSheet.getSprite("fire");

    auto burn = spriteSheet.getAnimationIndex("burn", "fire");
    m_animationMaps[SpriteID::Fire] = { burn };
    m_animationMaps[SpriteID::Fire][AnimationID::Die] = spriteSheet.getAnimationIndex("die", "fire");
    m_animationMaps[SpriteID::Fire][AnimationID::Spawn] = spriteSheet.getAnimationIndex("idle", "fire");

    spriteSheet.loadFromFile("assets/sprites/boat.spt", m_textureResource);
    m_sprites[SpriteID::Boat] = spriteSheet.getSprite("boat");
    m_animationMaps[SpriteID::Boat] = 
    {
        spriteSheet.getAnimationIndex("zero", "boat"),
        spriteSheet.getAnimationIndex("one", "boat"),
        spriteSheet.getAnimationIndex("two", "boat"),
        spriteSheet.getAnimationIndex("three", "boat"),
        spriteSheet.getAnimationIndex("four", "boat"),
        spriteSheet.getAnimationIndex("five", "boat"),
        0,0,0,0
    };
    m_sprites[SpriteID::Sail] = spriteSheet.getSprite("sail");

    spriteSheet.loadFromFile("assets/sprites/ships.spt", m_textureResource);
    m_sprites[SpriteID::Ships] = spriteSheet.getSprite("ship");
    m_sprites[SpriteID::ShipLights] = spriteSheet.getSprite("lights");

    spriteSheet.loadFromFile("assets/sprites/bees.spt", m_textureResource);
    m_sprites[SpriteID::Bees] = spriteSheet.getSprite("bees");
    m_animationMaps[SpriteID::Bees][AnimationID::WalkLeft] = spriteSheet.getAnimationIndex("left", "bees");
    m_animationMaps[SpriteID::Bees][AnimationID::WalkRight] = spriteSheet.getAnimationIndex("right", "bees");
    m_animationMaps[SpriteID::Bees][AnimationID::WalkDown] = spriteSheet.getAnimationIndex("sleep", "bees");
    m_animationMaps[SpriteID::Bees][AnimationID::WalkUp] = spriteSheet.getAnimationIndex("wake", "bees");

    m_sprites[SpriteID::Beehive] = spriteSheet.getSprite("hive");

    spriteSheet.loadFromFile("assets/sprites/decoy.spt", m_textureResource);
    m_sprites[SpriteID::Decoy] = spriteSheet.getSprite("decoy");
    m_animationMaps[SpriteID::Decoy][AnimationID::Spawn] = spriteSheet.getAnimationIndex("spawn", "decoy");
    m_animationMaps[SpriteID::Decoy][AnimationID::IdleDown] = spriteSheet.getAnimationIndex("idle", "decoy");
    m_animationMaps[SpriteID::Decoy][AnimationID::Die] = spriteSheet.getAnimationIndex("break", "decoy");

    m_sprites[SpriteID::DecoyItem] = spriteSheet.getSprite("decoy_item");

    spriteSheet.loadFromFile("assets/sprites/flare.spt", m_textureResource);
    m_sprites[SpriteID::Flare] = spriteSheet.getSprite("flare");
    m_sprites[SpriteID::FlareItem] = spriteSheet.getSprite("flare_item");
    m_sprites[SpriteID::SmokePuff] = spriteSheet.getSprite("smoke");

    auto flareItemCarried = spriteSheet.getAnimationIndex("carried", "flare_item");
    m_animationMaps[SpriteID::FlareItem][AnimationID::WalkDown] = flareItemCarried;
    m_animationMaps[SpriteID::FlareItem][AnimationID::WalkUp] = flareItemCarried;
    m_animationMaps[SpriteID::FlareItem][AnimationID::WalkLeft] = flareItemCarried;
    m_animationMaps[SpriteID::FlareItem][AnimationID::WalkRight] = flareItemCarried;

    spriteSheet.loadFromFile("assets/sprites/impact.spt", m_textureResource);
    m_sprites[SpriteID::Impact] = spriteSheet.getSprite("impact");

    spriteSheet.loadFromFile("assets/sprites/spooky_skull.spt", m_textureResource);
    m_sprites[SpriteID::SkullItem] = spriteSheet.getSprite("skull");
    m_sprites[SpriteID::SkullShield] = spriteSheet.getSprite("shield");
    m_animationMaps[SpriteID::SkullShield][AnimationID::Spawn] = spriteSheet.getAnimationIndex("spawn", "shield");
    m_animationMaps[SpriteID::SkullShield][AnimationID::IdleDown] = spriteSheet.getAnimationIndex("idle", "shield");
    m_animationMaps[SpriteID::SkullShield][AnimationID::Die] = spriteSheet.getAnimationIndex("despawn", "shield");

    m_audioScape.loadFromFile("assets/sound/game.xas");

    loadUI();
    loadAudio();
}

void GameState::loadScene(const TileArray& tileArray)
{
    if (m_sceneLoaded) return;
    
    //do this first so path finder data is accurate
    m_islandGenerator.render(tileArray, m_textureResource);

    m_pathFinder.setGridSize({ static_cast<int>(Global::TileCountX), static_cast<int>(Global::TileCountY) });
    m_pathFinder.setTileSize({ Global::TileSize, Global::TileSize });
    m_pathFinder.setGridOffset({ Global::TileSize / 2.f, Global::TileSize / 2.f });
    const auto& pathData = m_islandGenerator.getPathData();
    for (auto i = 0u; i < Global::TileCount; ++i)
    {
        //only sand tiles are traversable
        if ((pathData[i] > 60 && pathData[i] != 75) || (pathData[i] == 0 || pathData[i] == 15))
        {
            m_pathFinder.addSolidTile({ static_cast<int>(i % Global::TileCountX), static_cast<int>(i / Global::TileCountX) });
        }
    }

    //make boats impassable to path finder
    /*for (auto c : Global::TileExclusionCorners)
    {
        for (auto y = 0; y < Global::TileExclusionX; ++y)
        {
            for (auto x = 0; x < Global::TileExclusionY; ++x)
            {
                m_pathFinder.addSolidTile({ c.x + x, c.y + y });
            }
        }
    }*/


    //load from island generator
    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<Island>().texture = &m_islandGenerator.getTexture();
    entity.getComponent<Island>().normalMap = &m_islandGenerator.getNormalMap();
    entity.getComponent<Island>().waveMap = &m_islandGenerator.getWaveMap();

    m_miniMap.setTexture(m_islandGenerator.getTexture());

    //read foliage data from map generator
    m_foliageGenerator.generate(m_islandGenerator.getMapData(), m_gameScene);

    //apply map data to collision system
    m_gameScene.getSystem<CollisionSystem>().setTileData(tileArray);

    m_sceneLoaded = true;

    auto msg = getContext().appInstance.getMessageBus().post<MapEvent>(MessageID::MapMessage);
    msg->type = MapEvent::Loaded;

    m_sharedData.netClient->sendPacket(PacketID::RequestPlayer, std::uint8_t(0), xy::NetFlag::Reliable);

    //ships in the background
    auto cameraEntity = m_gameScene.getActiveCamera();

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(310.f, -520.f);
    entity.getComponent<xy::Transform>().setScale(1.f, -1.f);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Ships];
    entity.addComponent<Sprite3D>(m_modelMatrices);
    entity.addComponent<xy::Drawable>();    
    entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Ships].getTexture());
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);  
    auto& tx1 = entity.getComponent<xy::Transform>();

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, -2.f);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::ShipLights];
    entity.addComponent<Sprite3D>(m_modelMatrices);
    entity.addComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderUnlitTextured));
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::ShipLights].getTexture());
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ShipLight;
    tx1.addChild(entity.getComponent<xy::Transform>());


    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(1020.f, -560.f);
    entity.getComponent<xy::Transform>().setScale(-1.f, -1.f);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Ships];
    entity.addComponent<Sprite3D>(m_modelMatrices);
    entity.addComponent<xy::Drawable>();
    entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Ships].getTexture());
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    auto& tx2 = entity.getComponent<xy::Transform>();

    entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(0.f, -2.f);
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::ShipLights];
    entity.addComponent<Sprite3D>(m_modelMatrices);
    entity.addComponent<xy::Drawable>().setBlendMode(sf::BlendAdd);
    entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderUnlitTextured));
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::ShipLights].getTexture());
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    entity.addComponent<xy::CommandTarget>().ID = CommandID::ShipLight;
    tx2.addChild(entity.getComponent<xy::Transform>());

    //sprites for rain layers
    auto& rainTexture = m_textureResource.get("assets/images/rain.png");
    rainTexture.setRepeated(true);
    sf::Vector2f rainPos;

    for (auto i = 0; i < 3; ++i)
    {
        entity = m_gameScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(rainPos);
        entity.getComponent<xy::Transform>().setScale(1.f, -1.f);
        entity.addComponent<xy::Sprite>(rainTexture);
        entity.getComponent<xy::Sprite>().setTextureRect({ static_cast<float>(xy::Util::Random::value(0, 500)),
                                                        static_cast<float>(xy::Util::Random::value(0, 500)), Global::IslandSize.x, 1080.f });
        entity.addComponent<Sprite3D>(m_modelMatrices);
        entity.addComponent<xy::Drawable>();
        entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
        entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Sprite>().getTexture());
        entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.addComponent<xy::CommandTarget>().ID = CommandID::Rain;
        entity.addComponent<Rain>();

        rainPos.y += Rain::Spacing;
    }

    //starting message - maybe replace this with birds when we have them
    //entity = m_gameScene.createEntity();
    //entity.addComponent<xy::Transform>().setPosition(Global::IslandSize.x / 2.f, (Global::IslandSize.y / 2.f) - /*Global::PlayerCameraOffset*/200.f);
    //entity.getComponent<xy::Transform>().setScale(1.f, -1.f);
    //entity.getComponent<xy::Transform>().scale(0.25f, 0.25f);
    //entity.addComponent<xy::Sprite>(m_textureResource.get("assets/images/start.png"));
    //auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    //entity.getComponent<xy::Transform>().move(-(bounds.width / 2.f) * 0.25f, 0.f);
    //entity.addComponent<Sprite3D>(m_modelMatrices);
    //entity.addComponent<xy::Drawable>();
    //entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderUnlitTextured));
    //entity.getComponent<xy::Drawable>().bindUniform("u_texture", *entity.getComponent<xy::Sprite>().getTexture());
    //entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    //entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
}

void GameState::handlePacket(const xy::NetEvent& evt)
{
    switch (evt.packet.getID())
    {
    default: 
        //std::cout << (int)packet.id() << "\n";
        break;
    case PacketID::MapData:
    {
        const TileArray& mapData = evt.packet.as<TileArray>();
        loadScene(mapData);
    }
        break;
    case PacketID::ActorData:
    {
        const auto& state = evt.packet.as<ActorState>();
        if (m_inputParser.getPlayerEntity().getComponent<Actor>().serverID != state.serverID)
        {
            Actor actor;
            actor.id = static_cast<Actor::ID>(state.actorID);
            actor.serverID = state.serverID;
            spawnActor(actor, state.position, state.serverTime);
        }
    }
        break;
    case PacketID::PlayerData:
    {
        const auto& playerInfo = evt.packet.as<PlayerInfo>();
        spawnActor(playerInfo.actor, playerInfo.position, 0, true);
        m_miniMap.setLocalPlayer(playerInfo.actor.id - Actor::ID::PlayerOne);
    }
        break;
    case PacketID::ActorUpdate:
    {
        const auto& state = evt.packet.as<ActorState>();

        xy::Command cmd;
        cmd.targetFlags = CommandID::NetInterpolator;
        cmd.action = [state](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == state.serverID)
            {
                InterpolationPoint point;
                point.position = state.position;
                point.timestamp = state.serverTime;

                entity.getComponent<InterpolationComponent>().setTarget(point);
                entity.getComponent<Actor>().direction = state.direction;
            }
        };
        
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
        break;
    case PacketID::AnimationUpdate:
    {
        const auto& state = evt.packet.as<AnimationState>();

        xy::Command cmd;
        cmd.targetFlags = CommandID::NetInterpolator;
        cmd.action = [state](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == state.serverID)
            {
                auto& anim = entity.getComponent<AnimationModifier>();
                anim.nextAnimation = state.animation;
            }
        };

        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
        break;
    case PacketID::PlayerUpdate:
    {
        const auto& state = evt.packet.as<ClientState>();
        m_gameScene.getSystem<PlayerSystem>().reconcile(state, m_inputParser.getPlayerEntity());
    }
        break;
    case PacketID::DayNightUpdate:
    {
        auto* msg = getContext().appInstance.getMessageBus().post<MapEvent>(MessageID::MapMessage);
        msg->type = MapEvent::DayNightUpdate;
        msg->value = evt.packet.as<float>();
    }
        break;
    case PacketID::CarriableUpdate:
        updateCarriable(evt.packet.as<CarriableState>());
        break;
    case PacketID::InventoryUpdate:
        updateInventory(evt.packet.as<InventoryState>());
        break;
    case PacketID::SceneUpdate:
        updateScene(evt.packet.as<SceneState>());
        break;
    case PacketID::ConnectionUpdate:
        updateConnection(evt.packet.as<ConnectionState>());
        break;
    case PacketID::EndOfRound:
        m_roundOver = true;
        showRoundEnd(evt.packet.as<RoundSummary>());
        break;
    case PacketID::ServerMessage:
        showServerMessage(evt.packet.as<std::int32_t>());
        break;
    case PacketID::PathData:
    {
        /*auto data = packet.as<PathData>();
        std::vector<sf::Vector2f> pointData(data.data[0].x - 1);

        for (auto i = 0u; i < pointData.size(); ++i)
        {
            pointData[i] = data.data[i + 1];
        }
        plotPath(pointData);*/
    }
        break;
    case PacketID::PatchSync:
    {
        auto data = evt.packet.as<HoleState>();
        xy::Command cmd;
        cmd.targetFlags = CommandID::NetInterpolator;
        cmd.action = [data](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == data.serverID)
            {
                entity.getComponent<WetPatch>().state = static_cast<WetPatch::State>(data.state);
                entity.getComponent<WetPatch>().digCount = WetPatch::DigsPerStage * data.state;
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
        break;
    case PacketID::ParrotLaunch:
    {
        auto id = evt.packet.as<std::uint32_t>();
        xy::Command cmd;
        cmd.targetFlags = CommandID::Parrot;
        cmd.action = [id](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == id)
            {
                entity.getComponent<ParrotFlock>().start();
                entity.getComponent<xy::AudioEmitter>().play();
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
        break;
    case PacketID::StatUpdate:
    {
        auto stats = evt.packet.as<RoundStat>();
        m_summaryStats.stats[stats.id - Actor::ID::PlayerOne] = stats;
        m_updateSummary = true;
    }
        break;
    case PacketID::WeatherUpdate:
        m_gameScene.getSystem<DayNightSystem>().setStormLevel(evt.packet.as<std::uint8_t>());
        break;
    case PacketID::TreasureUpdate:
    {
        auto score = evt.packet.as<std::uint8_t>();
        xy::Command cmd;
        cmd.targetFlags = UI::CommandID::TreasureScore;
        cmd.action = [score](xy::Entity ent, float)
        {
            if (score < 2)
            {
                ent.getComponent<xy::Text>().setString(std::to_string(score) + " treasure remaining");
            }
            else
            {
                ent.getComponent<xy::Text>().setString(std::to_string(score) + " treasures remaining");
            }
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
        break;
    case PacketID::RoundTime:
        m_roundTime = evt.packet.as<std::int32_t>();
        break;
#ifdef XY_DEBUG
    case PacketID::DebugUpdate:
    {
        auto state = evt.packet.as<DebugState>();
        xy::Command cmd;
        cmd.targetFlags = CommandID::DebugLabel;
        cmd.action = [state](xy::Entity entity, float)
        {
            if (state.serverID == entity.getComponent<std::uint32_t>())
            {
                switch (state.state)
                {
                default:
                    entity.getComponent<xy::Text>().setString("Unknown State");
                    break;
                case 1:
                {
                    std::string target = state.target == 0 ? "Player" : std::to_string(state.target);

                    entity.getComponent<xy::Text>().setString("Walking " + std::to_string(state.pathSize) + " steps, target: " + target);
                }
                    break;
                case 2:
                    entity.getComponent<xy::Text>().setString("Dying");
                    break;
                }
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
    break;
#endif
    }
}

void GameState::updateCarriable(const CarriableState& state)
{
    if (state.action == CarriableState::PickedUp)
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::NetInterpolator;
        cmd.action = [&, state](xy::Entity ent, float)
        {
            if (ent.getComponent<Actor>().serverID == state.carriableID)
            {
                //ent is carriable item
                ent.getComponent<InterpolationComponent>().resetPosition(state.position);
                ent.getComponent<xy::Transform>().setPosition(state.position);

                //treasure which had been stashed will have had its depth put out of
                //sorting range so we need to correct that
                ent.getComponent<xy::Drawable>().setDepth(static_cast<sf::Int32>(state.position.y));

                xy::Command cmd2;
                cmd2.targetFlags = CommandID::NetInterpolator | CommandID::LocalPlayer;
                cmd2.action = [&, ent, state](xy::Entity ent2, float) mutable
                {
                    //ent2 is parent, ie player who picked it up
                    if (ent2.getComponent<Actor>().serverID == state.parentID)
                    {
                        ent.getComponent<Sprite3D>().verticalOffset = -3.f;
                        ent.getComponent<Carriable>().carried = true;
                        ent.getComponent<Carriable>().parentEntity = ent2;
                        ent.getComponent<xy::BroadphaseComponent>().setFilterFlags(0);
                        ent.getComponent<xy::Callback>().active = true;

                        ent2.getComponent<Actor>().carryingItem = true;

                        auto* msg = getContext().appInstance.getMessageBus().post<CarryEvent>(MessageID::CarryMessage);
                        msg->action = CarryEvent::PickedUp;
                        msg->type = ent.getComponent<Carriable>().type;
                        msg->entity = ent2;
                    }
                };
                m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd2);
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
    else if (state.action == CarriableState::Dropped)
    {
        xy::Command cmd;
        cmd.targetFlags = CommandID::NetInterpolator;
        cmd.action = [&, state](xy::Entity ent, float)
        {
            //ent is carriable item
            if (ent.getComponent<Actor>().serverID == state.carriableID)
            {
                ent.getComponent<InterpolationComponent>().resetPosition(state.position);
                ent.getComponent<xy::Transform>().setPosition(state.position);

                xy::Command cmd2;
                cmd2.targetFlags = CommandID::NetInterpolator | CommandID::LocalPlayer;
                cmd2.action = [&, ent, state](xy::Entity ent2, float) mutable
                {
                    //ent2 is parent, ie player
                    if (ent2.getComponent<Actor>().serverID == state.parentID) 
                    {
                        auto* msg = getContext().appInstance.getMessageBus().post<CarryEvent>(MessageID::CarryMessage);
                        msg->action = CarryEvent::Dropped;
                        //we can't set the type because it might not exist - but UI items can always assume
                        //that the carried item will be a treasure if it's already carried as players can only
                        //carry one thing at a time.
                        msg->entity = ent2;

                        if (!ent.destroyed()) //may have already been destroyed when placed in boat
                        {
                            auto& tx = ent.getComponent<xy::Transform>();
                            tx.setPosition(state.position);

                            ent.getComponent<Sprite3D>().verticalOffset = 0.f;
                            ent.getComponent<Carriable>().carried = false;
                            ent.getComponent<Carriable>().parentEntity = {};
                            ent.getComponent<xy::BroadphaseComponent>().setFilterFlags(QuadTreeFilter::BotQuery);
                            ent.getComponent<InterpolationComponent>().resetPosition(state.position);
                            ent.getComponent<xy::Callback>().active = false;
                        }
                        ent2.getComponent<Actor>().carryingItem = false;
                    }
                };
                m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd2);
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
    else
    {
        //fell in water so do some FX
        auto* msg = getContext().appInstance.getMessageBus().post<MapEvent>(MessageID::MapMessage);
        msg->type = MapEvent::ItemInWater;
        msg->position = state.position;
    }
}

void GameState::updateInventory(InventoryState state)
{
    auto localPlayer = m_inputParser.getPlayerEntity();
    if (state.parentID == localPlayer.getComponent<Actor>().serverID)
    {
        //this is us, update hud
        const auto& inventory = state.inventory;
        xy::Command cmd;
        cmd.targetFlags = UI::CommandID::WeaponSlot;
        cmd.action = [inventory](xy::Entity entity, float)
        {
            float width = entity.getComponent<xy::Sprite>().getTextureBounds().width;
            entity.getComponent<xy::Transform>().setPosition(inventory.weapon * width, 0.f);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = UI::CommandID::Ammo;
        cmd.action = [inventory](xy::Entity entity, float)
        {
            entity.getComponent<xy::Text>().setString(std::to_string(inventory.ammo));
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = UI::CommandID::Treasure;
        cmd.action = [inventory](xy::Entity entity, float)
        {
            entity.getComponent<xy::Text>().setString(std::to_string(inventory.treasure));
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        localPlayer.getComponent<Inventory>() = state.inventory;
    }
    else
    {
        //remote player info
        xy::Command cmd;
        cmd.targetFlags = CommandID::InventoryHolder;
        cmd.action = [state](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == state.parentID)
            {
                entity.getComponent<Inventory>() = state.inventory;
            }
        };
        m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }

    //and update any weapon sprite with selected weapon
    xy::Command cmd;
    cmd.targetFlags = CommandID::Weapon;
    cmd.action = [state](xy::Entity entity, float)
    {
        if (entity.getComponent<ClientWeapon>().parent.getComponent<Actor>().serverID == state.parentID)
        {
            entity.getComponent<ClientWeapon>().nextWeapon = state.inventory.weapon;
        }
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    //and update any health bars
    cmd.targetFlags = CommandID::HealthBar;
    cmd.action = [&, state](xy::Entity entity, float)
    {
        auto& healthBar = entity.getComponent<HealthBar>();
        if (healthBar.parent.isValid() && healthBar.parent.hasComponent<Actor>() &&
            healthBar.parent.getComponent<Actor>().serverID == state.parentID)
        {
            healthBar.health = state.inventory.health;
            auto id = healthBar.parent.getComponent<Actor>().id;

            if (healthBar.lastHealth > healthBar.health) //only flash when losing health
            {
                healthBar.parent.getComponent<xy::Sprite>().setColour(sf::Color(255, 120, 120));
                healthBar.parent.getComponent<xy::Callback>().active = true;

                //raise message with who got hurt by what, for weapon sounds
                //if (id >= Actor::ID::PlayerOne && id <= Actor::ID::PlayerFour)
                {
                    auto* msg = getContext().appInstance.getMessageBus().post<PlayerEvent>(MessageID::PlayerMessage);
                    msg->entity = healthBar.parent;
                    msg->action = static_cast<PlayerEvent::Action>(state.inventory.lastDamage);
                }
            }
            //repeat to the UI using actor ID
            xy::Command cmd2;
            cmd2.targetFlags = UI::CommandID::Health;
            cmd2.action = [id, state](xy::Entity e, float)
            {
                if (e.getComponent<std::uint32_t>() == id)
                {
                    e.getComponent<HealthBar>().health = state.inventory.health;
                }
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd2);
        }
    };
    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void GameState::updateScene(SceneState state)
{
    auto id = state.serverID;
    xy::Command cmd;
    if (state.action == SceneState::EntityRemoved)
    {
        cmd.targetFlags = CommandID::NetInterpolator | CommandID::Removable;
        cmd.action = [&, id](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == id)
            {
                auto* msg = getContext().appInstance.getMessageBus().post<ActorEvent>(MessageID::ActorMessage);
                msg->position = entity.getComponent<xy::Transform>().getPosition();
                msg->type = ActorEvent::Died;
                msg->id = entity.getComponent<Actor>().id;

                m_gameScene.destroyEntity(entity);
            }
        };
    }
    else if (state.action == SceneState::PlayerDied)
    {
        cmd.targetFlags = CommandID::PlayerAvatar;
        cmd.action = [&, id, state](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == id)
            {
                entity.getComponent<xy::Transform>().setScale(0.f, 0.f);
                spawnGhost(entity, state.position);

                //set camera to chase if local player
                if (entity == m_inputParser.getPlayerEntity())
                {
                    //m_gameScene.setSystemActive<Camera3DSystem>(false); //can't disable this as it messes up the matrix updates
                    m_gameScene.getSystem<Camera3DSystem>().setActive(false);
                }
                else
                {
                    //raise the dead message for remote player
                    auto msg = getContext().appInstance.getMessageBus().post<PlayerEvent>(MessageID::PlayerMessage);
                    msg->action = PlayerEvent::Died;
                    msg->entity = entity;
                    msg->position = state.position;
                }
            }
        };       
    }
    else if (state.action == SceneState::PlayerSpawned)
    {
        cmd.targetFlags = CommandID::PlayerAvatar;
        cmd.action = [&, id](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == id)
            {
                entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);

                if (entity == m_inputParser.getPlayerEntity())
                {
                    //m_gameScene.setSystemActive<Camera3DSystem>(true);
                    m_gameScene.getSystem<Camera3DSystem>().setActive(true);
                    m_gameScene.getSystem<Camera3DSystem>().enableChase(true);
                }
                else
                {
                    //raise message for remote players
                    auto* msg = getContext().appInstance.getMessageBus().post<PlayerEvent>(MessageID::PlayerMessage);
                    msg->action = PlayerEvent::Respawned;
                    msg->entity = entity;
                }
            }
        };
    }
    else if (state.action == SceneState::PlayerDidAction)
    {
        cmd.targetFlags = CommandID::NetInterpolator;
        cmd.action = [&, id](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == id)
            {
                auto* msg = getContext().appInstance.getMessageBus().post<PlayerEvent>(MessageID::PlayerMessage);
                msg->action = PlayerEvent::DidAction;
                msg->entity = entity;
            }
        };
    }
    else if (state.action == SceneState::StashedTreasure)
    {
        cmd.targetFlags = CommandID::PlayerAvatar;
        cmd.action = [&, id](xy::Entity entity, float)
        {
            if (entity.getComponent<Actor>().serverID == id)
            {
                auto* msg = getContext().appInstance.getMessageBus().post<PlayerEvent>(MessageID::PlayerMessage);
                msg->action = PlayerEvent::StashedTreasure;
                msg->entity = entity;
                msg->position = state.position;
                msg->data = id;
            }
        };
    }
    else if (state.action == SceneState::GotCursed)
    {
        LOG("got curse", xy::Logger::Type::Info);
        return;
    }
    else if (state.action == SceneState::LostCurse)
    {
        LOG("lost curse", xy::Logger::Type::Info);
        return;
    }
    else if (state.action == SceneState::Stung)
    {
        LOG("Stung by bees", xy::Logger::Type::Info);
    }

    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
}

void GameState::updateConnection(ConnectionState state)
{
    m_sharedData.clientInformation.updateClient(state);
    {
        auto idx = state.actorID - Actor::ID::PlayerOne;

        if (state.steamID == 0)
        {
            m_nameTagManager.updateName(Global::PlayerNames[idx], idx);
        }
        else
        {
            const auto& clientInfo = m_sharedData.clientInformation.getClient(state);
            m_nameTagManager.updateName(clientInfo.name, idx);
        }
    }
}

void GameState::spawnGhost(xy::Entity playerEnt, sf::Vector2f position)
{
    auto id = playerEnt.getComponent<Actor>().id - Actor::ID::PlayerOne;

    auto cameraEntity = m_gameScene.getActiveCamera();

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(position);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = m_sprites[SpriteID::Ghost];
    entity.addComponent<xy::SpriteAnimation>().play(id);
    entity.getComponent<xy::Drawable>().setShader(&m_shaderResource.get(ShaderID::SpriteShaderCulled));
    entity.getComponent<xy::Drawable>().bindUniform("u_texture", *m_sprites[SpriteID::Ghost].getTexture());
    entity.addComponent<Sprite3D>(m_modelMatrices);
    entity.getComponent<xy::Drawable>().bindUniform("u_viewProjMat", &cameraEntity.getComponent<Camera3D>().viewProjectionMatrix[0][0]);
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    entity.getComponent<xy::Transform>().setScale(0.5f, -0.5f);

    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, 0.f);

    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function = GhostFloatCallback(m_gameScene);
}

//void GameState::p2pSessionFail(P2PSessionConnectFail_t* cb)
//{
//    if (cb->m_steamIDRemote == m_sharedData.serverID)
//    {
//        m_inputParser.setEnabled(false);
//
//        //push error state
//        m_sharedData.error = "Disconnected from server.";
//        requestStackPush(StateID::Error);
//    }
//}