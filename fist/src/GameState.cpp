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

#include <glm/gtc/matrix_transform.hpp>

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>

#include <xyginext/util/Const.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>

namespace
{
#include "Sprite3DShader.inl"

    xy::Entity debugCam;
}

GameState::GameState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State(ss, ctx),
    m_gameScene(ctx.appInstance.getMessageBus())
{
    initScene();
    loadResources();
    loadMap(); //TODO store result then push error state on map fail
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
#ifdef XY_DEBUG
        case sf::Keyboard::Escape:
            {
                xy::App::quit();
            }
            break;
        case sf::Keyboard::F8:
            {
                auto newCam = debugCam;
                debugCam = m_gameScene.setActiveCamera(newCam);
            }
            break;
        case sf::Keyboard::A:
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
        case sf::Keyboard::D:
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

    m_gameScene.forwardEvent(evt);
    return true;
}

void GameState::handleMessage(const xy::Message& msg)
{
    if (msg.id == xy::Message::WindowMessage)
    {
        const auto& data = msg.getData<xy::Message::WindowEvent>();
        if (data.type == xy::Message::WindowEvent::Resized)
        {
            //reapply this as the context will have been recreated
            glFrontFace(GL_CW); 
        }
    }

    m_gameScene.forwardMessage(msg);
}

bool GameState::update(float dt)
{
    m_inputParser.update(dt);
    m_gameScene.update(dt);

    //updates the relevant shaders with the camera viewProj
    //TODO if we end up with multiple cameras move this to draw func
    //TODO use our piratey optimisation
    auto cam = m_gameScene.getActiveCamera();
    auto& shader = m_shaders.get(ShaderID::Sprite3DTextured);
    shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));

    return true;
}

void GameState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_gameScene);
}

xy::StateID GameState::stateID() const
{
    return StateID::Game;
}

//private
void GameState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_gameScene.addSystem<xy::CallbackSystem>(mb);
    m_gameScene.addSystem<xy::CommandSystem>(mb);
    m_gameScene.addSystem<CameraTransportSystem>(mb);
    m_gameScene.addSystem<Sprite3DSystem>(mb);
    m_gameScene.addSystem<xy::CameraSystem>(mb);
    m_gameScene.addSystem<Camera3DSystem>(mb);
    m_gameScene.addSystem<Render3DSystem>(mb);

    //m_uiScene.getActiveCamera().getComponent<xy::Camera>().setView(getContext().defaultView.getSize());
    //m_uiScene.getActiveCamera().getComponent<xy::Camera>().setViewport(getContext().defaultView.getViewport());

    //add a 3d camera  
    auto view = getContext().defaultView;
    auto camEnt = m_gameScene.getActiveCamera();
    camEnt.addComponent<xy::CommandTarget>().ID = CommandID::Camera;
    camEnt.getComponent<xy::Transform>().setPosition(0.f, GameConst::RoomWidth / 2.f); //TODO move cam further out and cull nearer rooms
    camEnt.addComponent<CameraTransport>();
    camEnt.getComponent<xy::Camera>().setView(view.getSize());
    camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());

    auto& camera = camEnt.addComponent<Camera3D>();
    float fov = camera.calcFOV(view.getSize().y, GameConst::RoomWidth / 2.f);
    float ratio = view.getSize().x / view.getSize().y;
    camera.projectionMatrix = glm::perspective(fov, ratio, 0.1f, GameConst::RoomWidth + GameConst::RoomHeight);
    camera.depth = GameConst::RoomHeight / 2.f;
    camera.rotationMatrix = glm::rotate(glm::mat4(1.f), -90.f * xy::Util::Const::degToRad, glm::vec3(1.f, 0.f, 0.f));
    m_gameScene.getSystem<Render3DSystem>().setCamera(camEnt);

#ifdef XY_DEBUG
    //set up a camera for deugging view
    debugCam = m_gameScene.createEntity();
    debugCam.addComponent<xy::Transform>().setPosition(xy::DefaultSceneSize / 2.f);
    debugCam.addComponent<xy::Camera>().setView(view.getSize());
    debugCam.getComponent<xy::Camera>().setViewport(view.getViewport());

    auto& dCamera = debugCam.addComponent<Camera3D>();
    fov = dCamera.calcFOV(view.getSize().y);
    dCamera.projectionMatrix = glm::perspective(fov, ratio, 0.1f, dCamera.depth + GameConst::RoomHeight);

    //temp just to move the camera about
    debugCam.addComponent<sf::Vector2f>();
    m_inputParser.setPlayerEntity(debugCam);
#endif
}

void GameState::loadResources()
{
    m_shaders.preload(ShaderID::Sprite3DTextured, SpriteVertex, SpriteFragmentTextured);
}