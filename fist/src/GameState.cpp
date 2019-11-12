/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "GameState.hpp"
#include "StateIDs.hpp"
#include "Camera3D.hpp"
#include "Sprite3D.hpp"
#include "ResourceIDs.hpp"
#include "GameConst.hpp"
#include "CommandIDs.hpp"
#include "CameraTransportSystem.hpp"
#include "Render3DSystem.hpp"
#include "RoomBuilder.hpp"
#include "PlayerDirector.hpp"
#include "AnimationID.hpp"
#include "SliderSystem.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/SpriteAnimation.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/systems/SpriteAnimator.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/UISystem.hpp>
#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/DynamicTreeSystem.hpp>

#include <xyginext/util/Const.hpp>
#include <xyginext/graphics/SpriteSheet.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>

namespace
{
#include "Sprite3DShader.inl"

    const sf::Vector2f InventoryOffPosition(0.f, -168.f);
    const sf::Vector2f InventoryOnPosition(0.f, 0.f);
    const sf::FloatRect InventoryButtonArea(855.f, 168.f, 210.f, 48.f);

    xy::Entity debugCam;

    std::int32_t startingRoom = 57;
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State (ss, ctx),
    m_gameScene (ctx.appInstance.getMessageBus(), 1024),
    m_uiScene   (ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();
    //TODO load current game state
    initScene();
    loadResources();
    loadMap(); //TODO store result then push error state on map fail
    
    addPlayer();

#ifdef XY_DEBUG
    debugSetup();
#endif
    buildUI();
    quitLoadingScreen();
}

//public
bool GameState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::ui::wantsKeyboard() || xy::ui::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
#ifdef XY_DEBUG
        case sf::Keyboard::Q:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Camera;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<CameraTransport>().rotate(true);
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
            break;
        case sf::Keyboard::E:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Camera;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<CameraTransport>().rotate(false);
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
        break;

        case sf::Keyboard::Escape:
            {
                xy::App::quit();
            }
            break;
        case sf::Keyboard::F3:
            requestStackPop();
            requestStackPush(StateID::Model);
            break;
        case sf::Keyboard::F8:
            {
                auto newCam = debugCam;
                debugCam = m_gameScene.setActiveCamera(newCam);
            }
            break;
        case sf::Keyboard::O:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Camera;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<CameraTransport>().move(true);
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
            break;
        case sf::Keyboard::P:
        {
            xy::Command cmd;
            cmd.targetFlags = CommandID::Camera;
            cmd.action = [](xy::Entity e, float)
            {
                e.getComponent<CameraTransport>().move(false);
            };
            m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
#endif //XY_DEBUG
        }
    }

    /*if (evt.type == sf::Event::MouseMoved)
    {
        auto pos = getContext().renderWindow.mapPixelToCoords({ evt.mouseMove.x, evt.mouseMove.y }, getContext().defaultView);
        std::cout << pos.x << ", " << pos.y << "\n";
    }*/

    m_gameScene.forwardEvent(evt);
    m_uiScene.getSystem<xy::UISystem>().handleEvent(evt);
    m_uiScene.forwardEvent(evt);
    return true;
}

void GameState::handleMessage(const xy::Message& msg)
{
    /*if (msg.id == xy::Message::WindowMessage)
    {
        const auto& data = msg.getData<xy::Message::WindowEvent>();
        if (data.type == xy::Message::WindowEvent::Resized)
        {
             
        }
    }*/

    m_gameScene.forwardMessage(msg);
    m_uiScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
#ifdef XY_DEBUG
    m_cameraInput.update(dt);
#endif
    m_gameScene.update(dt);
    m_uiScene.update(dt);

    //updates the relevant shaders with the camera viewProj
    //TODO if we end up with multiple cameras move this to draw func
    //TODO use our piratey optimisation
    auto cam = m_gameScene.getActiveCamera();
    auto& shader = m_shaders.get(ShaderID::Sprite3DTextured);
    shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));

    auto& shader2 = m_shaders.get(ShaderID::Sprite3DWalls);
    shader2.setUniform("u_viewMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewMatrix[0][0]));
    shader2.setUniform("u_projMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().projectionMatrix[0][0]));

    return true;
}

void GameState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_gameScene);
    rw.draw(m_uiScene);

//#ifdef XY_DEBUG
//    auto oldCam = m_gameScene.setActiveCamera(debugCam);
//    auto& shader = m_shaders.get(ShaderID::Sprite3DTextured);
//    shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&debugCam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));
//    rw.draw(m_gameScene);
//
//    shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&oldCam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));
//    m_gameScene.setActiveCamera(oldCam);
//#endif
}

xy::StateID GameState::stateID() const
{
    return StateID::Game;
}

//private
void GameState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<CameraTransportSystem>(mb);
    m_gameScene.addSystem<xy::SpriteAnimator>(mb);
    m_gameScene.addSystem<xy::SpriteSystem>(mb);
    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::DynamicTreeSystem>(mb);
    m_gameScene.addSystem<Sprite3DSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<Camera3DSystem>(mb);
    m_gameScene.addSystem<Render3DSystem>(mb);

    m_gameScene.addDirector<PlayerDirector>(m_uiScene);

    m_uiScene.addSystem<xy::UISystem>(mb);
    m_uiScene.addSystem<SliderSystem>(mb);
    m_uiScene.addSystem<xy::SpriteSystem>(mb);
    m_uiScene.addSystem<xy::TextSystem>(mb);
    m_uiScene.addSystem<xy::CameraSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);

    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setView(getContext().defaultView.getSize());
    m_uiScene.getActiveCamera().getComponent<xy::Camera>().setViewport(getContext().defaultView.getViewport());

    //add a 3d camera  
    auto view = getContext().defaultView;
    auto camEnt = m_gameScene.getActiveCamera();
    camEnt.addComponent<xy::CommandTarget>().ID = CommandID::Camera;
    camEnt.getComponent<xy::Transform>().setPosition(0.f, GameConst::RoomWidth * 0.49f);
    camEnt.addComponent<CameraTransport>(startingRoom); //57
    camEnt.getComponent<xy::Camera>().setView(view.getSize());
    camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());

    auto& camera = camEnt.addComponent<Camera3D>();
    float fov = camera.calcFOV(view.getSize().y * 1.3f, GameConst::RoomWidth / 2.f);
    float ratio = view.getSize().x / view.getSize().y;
    //camera.projectionMatrix = glm::ortho(-480.f, 480.f, -270.f, 270.f, 0.1f, GameConst::RoomWidth * 3.5f);
    camera.projectionMatrix = glm::perspective(fov, ratio, 0.1f, GameConst::RoomWidth * 6.5f);
    camera.depth = GameConst::RoomHeight / 3.f;
    camera.rotationMatrix = glm::rotate(glm::mat4(1.f), -90.f * xy::Util::Const::degToRad, glm::vec3(1.f, 0.f, 0.f));
    m_gameScene.getSystem<Render3DSystem>().setCamera(camEnt);
    //m_gameScene.getSystem<Render3DSystem>().setFOV(fov * ratio); //frustum width is in X plane

}

void GameState::loadResources()
{
    m_shaders.preload(ShaderID::Sprite3DTextured, SpriteVertexLighting, SpriteFragmentTextured);
    m_shaders.preload(ShaderID::Sprite3DColoured, SpriteVertexColoured, SpriteFragmentColoured);
    m_shaders.preload(ShaderID::Sprite3DWalls, SpriteVertexWalls, SpriteFragmentWalls);
}

void GameState::addPlayer()
{
    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/bob.spt", m_resources);

    auto createSprite = [&](const std::string sprite, sf::Vector2f position)->xy::Entity 
    {
        auto bounds = spriteSheet.getSprite(sprite).getTextureBounds();

        auto entity = m_gameScene.createEntity();
        entity.setLabel(sprite);
        entity.addComponent<xy::Transform>().setPosition(position);
        entity.getComponent<xy::Transform>().setOrigin(bounds.width / 2.f, bounds.height);
        entity.addComponent<xy::Sprite>() = spriteSheet.getSprite(sprite);
        entity.getComponent<xy::Sprite>().setColour({ 127, 255, 127 }); //normal direction
        const_cast<sf::Texture*>(entity.getComponent<xy::Sprite>().getTexture())->setSmooth(true);
        entity.addComponent<xy::SpriteAnimation>();
        entity.addComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().setDepth(100); //used by 3D render to depth sort
        entity.addComponent<Sprite3D>(m_matrixPool).depth = bounds.height;
        entity.getComponent<Sprite3D>().renderPass = 1;
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

        //hack to correct vert direction. Might move this to a system, particularly if we end up
        //using a bunch of animated sprites
        auto camEnt = m_gameScene.getActiveCamera();
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = [camEnt](xy::Entity e, float)
        {
            auto& verts = e.getComponent<xy::Drawable>().getVertices();
            verts[0].position = verts[1].position;
            verts[3].position = verts[2].position;
            verts[1].color.a = 0;
            verts[2].color.a = 0;

            //also want to read the camera's current rotation
            e.getComponent<xy::Transform>().setRotation(camEnt.getComponent<CameraTransport>().getCurrentRotation());
        };

        return entity;
    };

    auto x = startingRoom % GameConst::RoomsPerRow;
    auto y = startingRoom / GameConst::RoomsPerRow;

    sf::Vector2f position(x * GameConst::RoomWidth, y * GameConst::RoomWidth);
    auto player = createSprite("bob", position);
    m_gameScene.getDirector<PlayerDirector>().setPlayerEntity(player);
    m_gameScene.getDirector<PlayerDirector>().setCurrentRoom(startingRoom);

    auto& animMap = player.addComponent<AnimationMap>();
    animMap[AnimationID::Idle] = spriteSheet.getAnimationIndex("idle", "bob");
    animMap[AnimationID::Scratch] = spriteSheet.getAnimationIndex("scratch", "bob");
    animMap[AnimationID::InspectStart] = spriteSheet.getAnimationIndex("inspect_start", "bob");
    animMap[AnimationID::InspectEnd] = spriteSheet.getAnimationIndex("inspect_end", "bob");
    animMap[AnimationID::Left] = spriteSheet.getAnimationIndex("left", "bob");
    animMap[AnimationID::Right] = spriteSheet.getAnimationIndex("right", "bob");
    player.getComponent<xy::SpriteAnimation>().play(animMap[AnimationID::Idle]);


    //load bella
    spriteSheet.loadFromFile("assets/sprites/bella.spt", m_resources);
    position = { (x * GameConst::RoomWidth) - 100.f, (y * GameConst::RoomWidth) + 32.f };
    createSprite("bella", position);
}

void GameState::buildUI()
{
    //inventory bar
    auto texID = m_resources.load<sf::Texture>("assets/images/ui/inventory.png");
    auto& inventoryTexture = m_resources.get<sf::Texture>(texID);

    auto& uiSystem = m_uiScene.getSystem<xy::UISystem>();

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(InventoryOffPosition);
    entity.addComponent<xy::Drawable>().setDepth(-2);
    entity.addComponent<xy::Sprite>(inventoryTexture);
    entity.addComponent<Slider>().speed = 13.f;
    entity.getComponent<Slider>().target = InventoryOffPosition;
    entity.addComponent<xy::UIHitBox>().area = InventoryButtonArea;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([](xy::Entity e, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    auto& slider = e.getComponent<Slider>();
                    if (!slider.active)
                    {
                        if (slider.target.y == InventoryOnPosition.y)
                        {
                            slider.target.y = InventoryOffPosition.y;
                        }
                        else
                        {
                            slider.target.y = InventoryOnPosition.y;
                        }
                        slider.active = true;
                    }
                }
            });

    auto tabEnt = entity;
    auto mouseEnter = uiSystem.addMouseMoveCallback([](xy::Entity e, sf::Vector2f) 
        {
            auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
            bounds.top += bounds.height;
            e.getComponent<xy::Sprite>().setTextureRect(bounds);
        });

    auto mouseExit = uiSystem.addMouseMoveCallback([](xy::Entity e, sf::Vector2f)
        {
            auto bounds = e.getComponent<xy::Sprite>().getTextureRect();
            bounds.top = 0.f;
            e.getComponent<xy::Sprite>().setTextureRect(bounds);
        });

    //rotate left
    xy::SpriteSheet spriteSheet;
    spriteSheet.loadFromFile("assets/sprites/ui_icons.spt", m_resources);
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(20.f, 66.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("rot_left");
    auto bounds = entity.getComponent<xy::Sprite>().getTextureBounds();
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Camera;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<CameraTransport>().rotate(true);
                    };
                    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;

    tabEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

    //rotate right
    entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize.x - (20.f + bounds.width), 66.f);
    entity.addComponent<xy::Drawable>();
    entity.addComponent<xy::Sprite>() = spriteSheet.getSprite("rot_right");
    entity.addComponent<xy::UIHitBox>().area = bounds;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseUp] =
        uiSystem.addMouseButtonCallback([&](xy::Entity, sf::Uint64 flags)
            {
                if (flags & xy::UISystem::LeftMouse)
                {
                    xy::Command cmd;
                    cmd.targetFlags = CommandID::Camera;
                    cmd.action = [](xy::Entity e, float)
                    {
                        e.getComponent<CameraTransport>().rotate(false);
                    };
                    m_gameScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
                }
            });
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseEnter] = mouseEnter;
    entity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::MouseExit] = mouseExit;

    tabEnt.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());

}

#ifdef XY_DEBUG
void GameState::debugSetup()
{
    auto view = getContext().defaultView;

    //set up a camera for deugging view
    debugCam = m_gameScene.createEntity();
    debugCam.addComponent<xy::Transform>().setPosition(GameConst::RoomWidth, (GameConst::RoomWidth / 2.f) + GameConst::RoomWidth * 7);
    debugCam.addComponent<xy::Camera>().setView(view.getSize());
    debugCam.getComponent<xy::Camera>().setViewport(view.getViewport());

    auto& dCamera = debugCam.addComponent<Camera3D>();
    auto fov = dCamera.calcFOV(view.getSize().y * 8.f);
    float ratio = view.getSize().x / view.getSize().y;
    dCamera.projectionMatrix = glm::perspective(fov, ratio, 0.1f, dCamera.depth + GameConst::RoomHeight);

    //temp just to move the camera about
    debugCam.addComponent<sf::Vector2f>();
    m_cameraInput.setCameraEntity(debugCam);
    //debugCam = m_gameScene.setActiveCamera(debugCam);

    //and a debug icon
    auto texID = m_resources.load<sf::Texture>("assets/images/cam_debug.png");
    auto& tex = m_resources.get<sf::Texture>(texID);

    auto entity = m_gameScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().setTexture(&tex);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::RoomHeight / 2.f;
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    sf::Vertex vert;
    vert.color.a = 255;
    vert.position = { -32.f, -64.f };
    verts.push_back(vert);
    vert.position = { 32.f, -64.f };
    vert.texCoords = { 64.f, 0.f };
    verts.push_back(vert);
    vert.position = { 32.f, 0.f };
    vert.texCoords = { 64.f, 64.f };
    verts.push_back(vert);
    vert.position = { -32.f, 0.f };
    vert.texCoords = { 0.f, 64.f };
    verts.push_back(vert);
    std::reverse(verts.begin(), verts.end());
    entity.getComponent<xy::Drawable>().updateLocalBounds();

    auto camEnt = m_gameScene.getActiveCamera();
    entity.addComponent<xy::Callback>().active = true;
    entity.getComponent<xy::Callback>().function =
    [camEnt](xy::Entity e, float)
    {
        auto& tx = e.getComponent<xy::Transform>();
        const auto& cam = camEnt.getComponent<Camera3D>();
        tx.setPosition(cam.worldPosition.x, cam.worldPosition.y);
        tx.setRotation(camEnt.getComponent<CameraTransport>().getCurrentRotation());
    };
}
#endif