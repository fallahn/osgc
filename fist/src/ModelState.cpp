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
#include "ModelIO.hpp"
#include "SharedStateData.hpp"

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

    std::array<std::string, 4u> viewStrings =
    {
        "North", "East", "South", "West"
    };
}

ModelState::ModelState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_modelLoaded        (false),
    m_showModelImporter (false),
    m_quitEditor        (false),
    m_currentView       (0)
{
    initScene();

    //model importer
    registerWindow([&]()
        {
            if (m_showModelImporter)
            {
                ImGui::SetNextWindowSize({ 400, 400 }, ImGuiCond_FirstUseEver);
                if (ImGui::Begin("Model Importer", &m_showModelImporter))
                {
                    if (ImGui::Button("Import"))
                    {
                        auto filePath = xy::FileSystem::openFileDialogue(xy::FileSystem::getResourcePath(), "obj");
                        if (!filePath.empty() && m_objLoader.LoadFile(filePath))
                        {
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

                            m_modelInfo = ss.str();
                            m_modelLoaded = true;
                        }
                    }
                    ImGui::SameLine();
                    static bool importImmediate = false;
                    if (ImGui::Button("Save") && m_modelLoaded)
                    {
                        auto filePath = xy::FileSystem::saveFileDialogue(xy::FileSystem::getResourcePath(), "xym");
                        if (!filePath.empty())
                        {
                            if (exportModel(filePath) && importImmediate)
                            {
                                //TODO load the exported model to the scene
                            }
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Checkbox("Load to scene on save", &importImmediate);

                    if (m_modelLoaded)
                    {
                        ImGui::Text("%s", m_modelInfo.data());
                    }
                    else
                    {
                        ImGui::Text("%s", "No Model Loaded.");
                    }

                    if (ImGui::Button("Reset Rotation")
                        && m_modelLoaded)
                    {
                        modelEntity.getComponent<xy::Transform>().setRotation(0.f);
                    }
                }
                ImGui::End();
            }
        });

    //main window
    registerWindow([&]()
        {
            ImGui::SetNextWindowSize({ 400, 400 }, ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar))
            {
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("File"))
                    {
                        ImGui::MenuItem("Save", nullptr, nullptr);
                        ImGui::MenuItem("Quit", nullptr, &m_quitEditor);
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Tools"))
                    {
                        ImGui::MenuItem("Import obj", nullptr, &m_showModelImporter);
                        ImGui::MenuItem("Add model", nullptr, nullptr);
                        //TODO launch the baker from here somehow?
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                //instructions
                ImGui::Text("%s", "1-4 Change view");

                ImGui::Text("%s", "Current view :");
                ImGui::SameLine();
                auto lastView = m_currentView;
                if (ImGui::BeginCombo("direction", viewStrings[m_currentView].c_str(), ImGuiComboFlags_NoArrowButton))
                {
                    for (auto i = 0u; i < viewStrings.size(); ++i)
                    {
                        bool selected = (m_currentView == i);
                        if (ImGui::Selectable(viewStrings[i].c_str(), selected))
                        {
                            m_currentView = i;
                        }
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                if (lastView != m_currentView)
                {
                    setCamera(m_currentView);
                }
                ImGui::Separator();
                ImGui::Text("%s", "Current Room: ");
                ImGui::SameLine();
                ImGui::Text("%s", std::to_string(m_sharedData.currentRoom).c_str());
                
                //set room texture
                if (ImGui::Button("Select Texture"))
                {
                    auto path = xy::FileSystem::openFileDialogue(xy::FileSystem::getResourcePath(), "png,jpg");
                    if (!path.empty())
                    {
                        if (xy::FileSystem::fileExists(path))
                        {
                            m_roomTexture.loadFromFile(path);

                            std::replace(path.begin(), path.end(), '\\', '/');
                            if (auto idx = path.find("assets"); idx != std::string::npos)
                            {
                                path = path.substr(idx);

                                if (auto* src = m_roomList[m_sharedData.currentRoom]->findProperty("src"); src)
                                {
                                    src->setValue(path);
                                }
                                else
                                {
                                    m_roomList[m_sharedData.currentRoom]->addProperty("src").setValue(path);
                                }
                            }
                            else
                            {
                                xy::Logger::log("\'assets\' was not found in texture path, path nor saved to map data");
                            }
                        }
                    }
                }
                
                //TODO add/remove walls
                //TODO add/remove doors if wall exists

                //TODO list models in scene
                //TODO show info on selected model
            }
            ImGui::End();

            if (m_quitEditor)
            {
                //TODO confirmation
                //TODO remind to bake?

                m_mapData.save(xy::FileSystem::getResourcePath() + "assets/game.map");

                requestStackPop();
                requestStackPush(StateID::Game);
            }
        });
}

//public
bool ModelState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	if (/*xy::ui::wantsKeyboard() || */xy::ui::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::Escape:
#ifdef XY_DEBUG
            xy::App::quit();
#else
            //TODO display quit without saving dialogue

            requestStackClear();
            requestStackPush(StateID::MainMenu);
#endif
            break;
        case sf::Keyboard::F3:
            requestStackPop();
            requestStackPush(StateID::Game);
            //TODO ask to save?
            break;
        case sf::Keyboard::Num1:
            setCamera(0);
            m_currentView = 0;
            break;
        case sf::Keyboard::Num2:
            setCamera(1);
            m_currentView = 1;
            break;
        case sf::Keyboard::Num3:
            setCamera(2);
            m_currentView = 2;
            break;
        case sf::Keyboard::Num4:
            setCamera(3);
            m_currentView = 3;
            break;
        }
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
    if (m_modelLoaded)
    {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            modelEntity.getComponent<xy::Transform>().rotate(120.f * dt);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            modelEntity.getComponent<xy::Transform>().rotate(-120.f * dt);
        }
    }

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
    m_roomTexture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/rooms/editor.png");

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
    entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
    entity.getComponent<xy::Drawable>().setTexture(&m_roomTexture);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.addComponent<Sprite3D>(m_matrixPool).depth = GameConst::RoomHeight;
    entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);

    m_roomEntity = entity;

    if (!m_mapData.loadFromFile(xy::FileSystem::getResourcePath() + "assets/game.map"))
    {
        //TODO push error state

        return;
    }

    auto& objects = m_mapData.getObjects();
    for (auto& obj : objects)
    {
        if (obj.getName() == "room")
        {
            m_roomList.push_back(&obj);
        }
    }

    if (m_roomList.size() != 64)
    {
        //TODO push error state
        xy::Logger::log("Room data missing from map file - only " + std::to_string(m_roomList.size()) + " rooms were found", xy::Logger::Type::Error);
        return;
    }

    bool sortFailed = false;
    std::sort(m_roomList.begin(), m_roomList.end(),
        [&](const xy::ConfigObject* a, const xy::ConfigObject* b)
        {
            int IDa = 0;
            int IDb = 0;
            try
            {
                IDa = std::atoi(a->getId().c_str());
                IDb = std::atoi(b->getId().c_str());
            }
            catch (...) { sortFailed = true; }

            return IDa < IDb;
        });

    if (sortFailed)
    {
        //TODO push error state

        return;
    }

    setRoomGeometry();
}

void ModelState::parseVerts()
{
    if (!m_modelLoaded)
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
            vert.position.y = -meshVerts[i].Position.Y;

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
            vert.color.g = convertComponent(-meshVerts[i].Normal.Y);
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

bool ModelState::exportModel(const std::string& path)
{
    //assert path is into assets directory
    if (auto idx = path.find("assets"); idx != std::string::npos)
    {
        auto absPath = path;
        if (xy::FileSystem::getFileExtension(path) != ".xym")
        {
            absPath += ".xym";
        }

        auto relPath = absPath.substr(idx);
        std::replace(relPath.begin(), relPath.end(), '\\', '/');

        //serialise verts and write binary mesh file
        if (writeModelBinary(modelEntity.getComponent<xy::Drawable>().getVertices(), absPath))
        {
            //write metadata file containing bin name,
            //texture name and pos/rot/scale
            xy::ConfigFile cfg("model");
            cfg.addProperty("bin").setValue(relPath);
            cfg.addProperty("texture").setValue(std::string("")); //TODO import texture path from mtl
            cfg.addProperty("position").setValue(sf::Vector2f());
            cfg.addProperty("rotation").setValue(0.f);
            cfg.addProperty("scale").setValue(sf::Vector2f(1.f, 1.f));
            cfg.save(path.substr(0, -3) + ".xmd");

            modelEntity.getComponent<xy::Drawable>().getVertices().clear();
            m_modelLoaded = false;

            return true;
        }
    }
    return false;
}

void ModelState::setCamera(std::int32_t direction)
{
    XY_ASSERT(direction < 4 && direction > -1, "out of range");

    static const std::array<float, 4u> rotations =
    {
        0.f,
        90.f * xy::Util::Const::degToRad,
        xy::Util::Const::PI,
        -90.f * xy::Util::Const::degToRad
    };

    static const std::array<sf::Vector2f, 4u> positions =
    {
        sf::Vector2f(0.f, GameConst::RoomWidth),
        {-GameConst::RoomWidth, 0.f},
        {0.f, -GameConst::RoomWidth},
        {GameConst::RoomWidth, 0.f}
    };

    auto& cam = m_uiScene.getActiveCamera().getComponent<Camera3D>();
    cam.rotationMatrix = glm::rotate(glm::mat4(1.f), -90.f * xy::Util::Const::degToRad, glm::vec3(1.f, 0.f, 0.f));
    cam.rotationMatrix = glm::rotate(cam.rotationMatrix, rotations[direction], glm::vec3(0.f, 0.f, 1.f));

    m_uiScene.getActiveCamera().getComponent<xy::Transform>().setPosition(positions[direction]);
}

void ModelState::setRoomGeometry()
{
    XY_ASSERT(m_sharedData.currentRoom > 0 && m_sharedData.currentRoom < m_roomList.size(), "Invalid Index");

    auto& room = m_roomList[m_sharedData.currentRoom];
    const auto& properties = room->getProperties();
    
    std::int32_t flags = 0;
    for (const auto& prop : properties)
    {
        //TODO look for texture path
        if (prop.getName() == "ceiling"
            && prop.getValue<std::int32_t>() != 0)
        {
            flags |= WallFlags::Ceiling;
        }
        else if (prop.getName() == "src"
            && xy::FileSystem::fileExists(xy::FileSystem::getResourcePath() + prop.getValue<std::string>()))
        {
            m_roomTexture.loadFromFile(xy::FileSystem::getResourcePath() + prop.getValue<std::string>());
        }
    }

    const auto& objects = room->getObjects();
    for (const auto& obj : objects)
    {
        if (obj.getName() == "wall")
        {
            const auto& wallProperties = obj.getProperties();
            for (const auto& wallProp : wallProperties)
            {
                if (wallProp.getName() == "direction")
                {
                    switch (wallProp.getValue<std::int32_t>())
                    {
                    default: break;
                    case 0:
                        flags |= WallFlags::North;
                        break;
                    case 1:
                        flags |= WallFlags::East;
                        break;
                    case 2:
                        flags |= WallFlags::South;
                        break;
                    case 3:
                        flags |= WallFlags::West;
                        break;
                    }
                }
            }
        }
    }

    auto& verts = m_roomEntity.getComponent<xy::Drawable>().getVertices();
    verts.clear();
    addFloor(verts);

    for (auto i = 0; i < 4; ++i)
    {
        if (flags & (1 << i))
        {
            addWall(verts, i);
        }
    }
    if (flags & WallFlags::Ceiling)
    {
        addCeiling(verts);
    }

    m_roomEntity.getComponent<xy::Drawable>().updateLocalBounds();
}