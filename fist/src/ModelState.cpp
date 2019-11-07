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

#include "ModelState.hpp"
#include "StateIDs.hpp"
#include "Sprite3D.hpp"
#include "Camera3D.hpp"
#include "ResourceIDs.hpp"
#include "GameConst.hpp"
#include "RoomBuilder.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/gui/Gui.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/util/Vector.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/OpenGL.hpp>

#include <sstream>

namespace
{
#include "Sprite3DShader.inl"
    xy::Entity modelEntity;
}

ModelState::ModelState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State (ss, ctx),
    m_uiScene   (ctx.appInstance.getMessageBus()),
    m_fileLoaded(false)
{
    initScene();

    registerWindow([&]()
        {
            ImGui::SetNextWindowSize({ 400, 400 }, ImGuiCond_FirstUseEver);
            ImGui::Begin("Model Importer");
            if (ImGui::Button("Import"))
            {
                auto filePath = xy::FileSystem::openFileDialogue(xy::FileSystem::getResourcePath(), "obj");
                if (!filePath.empty() && m_objLoader.LoadFile(filePath))
                {
                    if (m_fileLoaded)
                    {
                        //TODO delete existing entity
                    }

                    std::stringstream ss;
                    ss << "Model Info:\n\n";
                    for (const auto& mesh : m_objLoader.LoadedMeshes)
                    {
                        ss << mesh.MeshName << "\n";
                        ss << "Vertices: " << mesh.Vertices.size() << "\n";
                        ss << "Triangles: " << mesh.Indices.size() / 3 << "\n";
                        ss << "Material: " << mesh.MeshMaterial.name << "\n\n";
                    }

                    parseVerts();

                    m_fileData = ss.str();
                    m_fileLoaded = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Export"))
            {
                auto filePath = xy::FileSystem::saveFileDialogue(xy::FileSystem::getResourcePath(), "xym");
                if (!filePath.empty())
                {
                    exportModel(filePath);
                }
            }

            if (m_fileLoaded)
            {
                ImGui::Text("%s", m_fileData.data());
            }
            else
            {
                ImGui::Text("%s", "No Model Loaded.");
            }

            ImGui::End();
        });
}

//public
bool ModelState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (xy::ui::wantsKeyboard() || xy::ui::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased
        && evt.key.code == sf::Keyboard::Escape)
    {
        xy::App::quit();
    }

    m_uiScene.forwardEvent(evt);
    return false;
}

void ModelState::handleMessage(const xy::Message& msg)
{
    m_uiScene.forwardMessage(msg);
}

bool ModelState::update(float dt)
{
    auto cam = m_uiScene.getActiveCamera();
    auto& shader = m_shaders.get(ShaderID::Sprite3DTextured);
    shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));

    m_uiScene.update(dt);
    return false;
}

void ModelState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_uiScene);
}

xy::StateID ModelState::stateID() const
{
    return StateID::Model;
}

//private
void ModelState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_uiScene.addSystem<xy::CallbackSystem>(mb);
    m_uiScene.addSystem<Sprite3DSystem>(mb);
    m_uiScene.addSystem<Camera3DSystem>(mb);
    m_uiScene.addSystem<xy::CameraSystem>(mb);
    m_uiScene.addSystem<xy::RenderSystem>(mb);

    auto view = getContext().defaultView;
    auto camEnt = m_uiScene.getActiveCamera();
    camEnt.getComponent<xy::Camera>().setView(view.getSize());
    camEnt.getComponent<xy::Camera>().setViewport(view.getViewport());
    camEnt.getComponent<xy::Transform>().setPosition(0.f, GameConst::RoomWidth);

    auto& camera = camEnt.addComponent<Camera3D>();
    float fov = camera.calcFOV(view.getSize().y, GameConst::RoomWidth / 2.f);
    float ratio = view.getSize().x / view.getSize().y;
    camera.projectionMatrix = glm::perspective(fov, ratio, 0.1f, GameConst::RoomWidth * 3.5f);
    camera.rotationMatrix = glm::rotate(glm::mat4(1.f), -90.f * xy::Util::Const::degToRad, glm::vec3(1.f, 0.f, 0.f));
    camera.depth = GameConst::RoomHeight / 2.f;

    //load shader
    m_shaders.preload(ShaderID::Sprite3DTextured, SpriteVertexLighting, SpriteFragmentTextured);

    //set up a background
    auto texID = m_resources.load<sf::Texture>("assets/images/rooms/template.png");
    auto& tex = m_resources.get<sf::Texture>(texID);

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
    entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
    entity.getComponent<xy::Drawable>().setTexture(&tex);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::RoomHeight;
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
    
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();
    addFloor(verts);
    addCeiling(verts);
    addWall(verts, 0);
    addWall(verts, 1);
    addWall(verts, 3);

    entity.getComponent<xy::Drawable>().updateLocalBounds();
}

void ModelState::parseVerts()
{
    if (!m_fileLoaded)
    {
        auto texID = m_resources.load<sf::Texture>("assets/images/cam_debug.png");
        auto& tex = m_resources.get<sf::Texture>(texID);

        //create the entity first
        modelEntity = m_uiScene.createEntity();
        modelEntity.addComponent<xy::Transform>();
        modelEntity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        modelEntity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
        modelEntity.getComponent<xy::Drawable>().setTexture(&tex);
        modelEntity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        modelEntity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        modelEntity.getComponent<xy::Drawable>().setCulled(false);
        modelEntity.getComponent<xy::Drawable>().setPrimitiveType(sf::Triangles);
        modelEntity.addComponent<Sprite3D>(m_matrixPool);
        modelEntity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &modelEntity.getComponent<Sprite3D>().getMatrix()[0][0]);
    }

    //update the entity's vert data
    auto& verts = modelEntity.getComponent<xy::Drawable>().getVertices();
    verts.clear();

    if (!m_objLoader.LoadedMeshes.empty()
        && !m_objLoader.LoadedMeshes[0].Vertices.empty()
        && !m_objLoader.LoadedMeshes[0].Indices.empty())
    {
        const auto& meshVerts = m_objLoader.LoadedMeshes[0].Vertices;
        const auto& meshIndices = m_objLoader.LoadedMeshes[0].Indices;

        auto zExtremes = std::minmax_element(meshVerts.begin(), meshVerts.end(),
            [](const objl::Vertex& lhs, const objl::Vertex& rhs)
            {
                return lhs.Position.Z < rhs.Position.Z;
            });
        float range = zExtremes.second->Position.Z - zExtremes.first->Position.Z;

        modelEntity.getComponent<Sprite3D>().depth = range;

        //add minheight to all z vals to make sure model starts on ground..?
        for (auto i : meshIndices)
        {
            sf::Vertex vert;
            //positions
            vert.position.x = meshVerts[i].Position.X;
            vert.position.y = meshVerts[i].Position.Y;

            float z = meshVerts[i].Position.Z - zExtremes.first->Position.Z;
            z = z / range;
            vert.color.a = static_cast<sf::Uint8>(z * 255.f);

            //normals
            auto convertComponent = [](float c)->sf::Uint8
            {
                c *= 255.f;
                c += 255.f;
                c /= 2.f;
                return static_cast<sf::Uint8>(c);
            };
            vert.color.r = convertComponent(meshVerts[i].Normal.X);
            vert.color.g = convertComponent(meshVerts[i].Normal.Y);
            vert.color.b = convertComponent(meshVerts[i].Normal.Z);

            //texcoords - for now we'll leave them normalised
            //because SFML uses texture sizes (hum) then remember to
            //convert these when we load the model in game and know
            //how big our texture is
            vert.texCoords.x = meshVerts[i].TextureCoordinate.X;
            vert.texCoords.y = meshVerts[i].TextureCoordinate.Y;
            
            verts.push_back(vert);
        }

        modelEntity.getComponent<xy::Drawable>().updateLocalBounds();
    }
    else
    {
        xy::Logger::log("Something went wrong parsing vertices", xy::Logger::Type::Error);
    }
}

void ModelState::exportModel(const std::string& path)
{

}