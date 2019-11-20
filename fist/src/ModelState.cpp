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

    struct NodeData final
    {
        std::string binPath;
        std::string texture;
    };
}

ModelState::ModelState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State         (ss, ctx),
    m_sharedData        (sd),
    m_uiScene           (ctx.appInstance.getMessageBus()),
    m_modelLoaded       (false),
    m_defaultTexID      (0),
    m_showModelImporter (false),
    m_showAerialView    (true),
    m_quitEditor        (false),
    m_loadModel         (false),
    m_saveRoom          (false),
    m_currentView       (0),
    m_wallFlags         (0)
{
    initScene();

    //aerial view
    registerWindow([&]() 
        {
            if (m_showAerialView)
            {
                ImGui::SetNextWindowSize({ 220.f, 250.f }, ImGuiCond_FirstUseEver);
                ImGui::SetWindowPos({ 15.f, 15.f }, ImGuiCond_FirstUseEver);
                ImGui::Begin("Aerial View", &m_showAerialView);
                ImGui::Image(m_aerialSprite, sf::Color::White, sf::Color::White);
                ImGui::End();
            }
        });

    //model importer
    registerWindow([&]()
        {
            if (m_showModelImporter)
            {
                ImGui::SetNextWindowSize({ 312.f, 200.f }, ImGuiCond_FirstUseEver);
                ImGui::SetWindowPos({ 15.f, 240.f }, ImGuiCond_FirstUseEver);
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
                                //load the exported model to the scene
                                loadModel(filePath);
                            }
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Checkbox("Load to scene on save", &importImmediate);

                    if (m_modelLoaded)
                    {
                        ImGui::Text("%s", m_modelInfo.c_str());
                        ImGui::Text("%s", m_modelTextureName.c_str());
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
            ImGui::SetNextWindowSize({ 300.f, 540.f }, ImGuiCond_FirstUseEver);
            ImGui::SetWindowPos({ 420.f, 16.f }, ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar))
            {
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("File"))
                    {
                        ImGui::MenuItem("Save", nullptr, &m_saveRoom);
                        ImGui::MenuItem("Quit", nullptr, &m_quitEditor);
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Tools"))
                    {
                        ImGui::MenuItem("Import obj", nullptr, &m_showModelImporter);
                        ImGui::MenuItem("Add model", nullptr, &m_loadModel);
                        //TODO launch the baker from here somehow?
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Window"))
                    {
                        ImGui::MenuItem("Aerial View", nullptr, &m_showAerialView);
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
                ImGui::SameLine();
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
                
                if (ImGui::Button("Prev") && m_sharedData.currentRoom > 0)
                {
                    m_sharedData.currentRoom--;
                    setRoomGeometry();
                }
                ImGui::SameLine();
                if (ImGui::Button("Next") && m_sharedData.currentRoom < 63)
                {
                    m_sharedData.currentRoom++;
                    setRoomGeometry();
                }

                //TODO add/remove walls
                //TODO add/remove doors if wall exists

                ImGui::Separator();

                //list models in scene
                if (!m_modelList.empty())
                {
                    ImGui::ListBoxHeader("Models");
                    for (const auto& [name, ent] : m_modelList)
                    {
                        if (ImGui::Selectable(name.c_str(), ent == m_selectedModel))
                        {
                            //unset old colour if something already selected
                            if (m_selectedModel.isValid())
                            {
                                m_selectedModel.getComponent<xy::Drawable>().bindUniform("u_highlightColour", sf::Vector3f(1.f, 1.f, 1.f));
                            }

                            //set selected entity
                            m_selectedModel = ent;
                            m_selectedModel.getComponent<xy::Drawable>().bindUniform("u_highlightColour", sf::Vector3f(0.f, 1.f, 0.f));
                        }
                    }
                    ImGui::ListBoxFooter();
                }

                //show info on selected model
                if (m_selectedModel.isValid())
                {
                    auto& tx = m_selectedModel.getComponent<xy::Transform>();
                    auto pos = tx.getPosition();
                    auto rotation = tx.getRotation();

                    ImGui::SliderFloat2("Position", reinterpret_cast<float*>(&pos), -GameConst::RoomWidth / 2.f, GameConst::RoomWidth / 2.f);
                    ImGui::SliderFloat("Rotation", &rotation, -180.f, 180.f);

                    tx.setPosition(pos);
                    tx.setRotation(rotation);

                    if (ImGui::Button("Delete"))
                    {
                        auto entry = std::find_if(m_modelList.begin(), m_modelList.end(),
                            [&](const std::pair<std::string, xy::Entity>& p)
                            { 
                                return p.second == m_selectedModel;
                            });
                        m_modelList.erase(entry);                        

                        m_uiScene.destroyEntity(m_selectedModel);
                        m_selectedModel = {};
                    }
                }
            }
            ImGui::End();

            if (m_quitEditor)
            {
                //TODO confirmation
                //TODO remind to bake?

                saveRoom();

                requestStackPop();
                requestStackPush(StateID::Game);
            }

            if (m_loadModel)
            {
                auto path = xy::FileSystem::openFileDialogue(xy::FileSystem::getResourcePath(), "xmd");
                if(!path.empty())
                {
                    loadModel(path);
                }

                m_loadModel = false;
            }

            if (m_saveRoom)
            {
                saveRoom();
                m_saveRoom = false;
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
            saveRoom();
            requestStackPop();
            requestStackPush(StateID::Game);
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
    else if (evt.type == sf::Event::MouseButtonReleased)
    {
        if (evt.mouseButton.button == sf::Mouse::Left)
        {
            //deselect any selection
            if (m_selectedModel.isValid())
            {
                m_selectedModel.getComponent<xy::Drawable>().bindUniform("u_highlightColour", sf::Vector3f(1.f, 1.f, 1.f));
            }
            m_selectedModel = {};
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

    if (!m_modelList.empty() && m_showAerialView)
    {
        m_aerialObjects.clear();

        for (const auto& model : m_modelList)
        {
            sf::RectangleShape& rect = m_aerialObjects.emplace_back();
            auto aabb = model.second.getComponent<xy::Drawable>().getLocalBounds();
            aabb = model.second.getComponent<xy::Transform>().getTransform().transformRect(aabb);
            rect.setSize({ aabb.width, aabb.height });
            rect.setFillColor(model.second == m_selectedModel ? sf::Color::Green : sf::Color(100,149,237));
            rect.setOrigin(rect.getSize() / 2.f);
            rect.setPosition(model.second.getComponent<xy::Transform>().getPosition());
        }
    }

    m_uiScene.update(dt);
    return false;
}

void ModelState::draw()
{
    if (m_showAerialView)
    {
        m_aerialPreview.clear(sf::Color(10, 20, 60));
        m_aerialPreview.draw(m_aerialBackground);

        for (const auto& r : m_aerialWalls)
        {
            m_aerialPreview.draw(r);
        }

        for (const auto& r : m_aerialObjects)
        {
            m_aerialPreview.draw(r);
        }
        m_aerialPreview.draw(m_eyeconOuter);
        m_aerialPreview.display();  
    }

    auto& rw = getContext().renderWindow;
    auto cam = m_uiScene.getActiveCamera();    
    auto& shader = m_shaders.get(ShaderID::Sprite3DTextured);
    shader.setUniform("u_viewProjMat", sf::Glsl::Mat4(&cam.getComponent<Camera3D>().viewProjectionMatrix[0][0]));
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

    //top down preview
    m_aerialPreview.create(200, 200, sf::ContextSettings(24u));
    m_aerialPreview.setView(sf::View(sf::Vector2f(), sf::Vector2f(1020.f, 1020.f)));
    m_aerialSprite.setTexture(m_aerialPreview.getTexture(), true);
    //flippedy doo dah
    auto rect = m_aerialSprite.getTextureRect();
    rect.top = rect.height;
    rect.height = -rect.height;
    m_aerialSprite.setTextureRect(rect);

    m_aerialBackground.setSize({ GameConst::RoomWidth, GameConst::RoomWidth });
    m_aerialBackground.setOrigin(m_aerialBackground.getSize() / 2.f);
    m_aerialBackground.setFillColor(sf::Color(255, 255, 255, 80));

    m_eyeconOuter.setRadius(0.5f);
    m_eyeconOuter.setOrigin(0.5f, 0.5f);
    m_eyeconOuter.setScale(28.f, 14.f);
    m_eyeconOuter.setFillColor(sf::Color::Black);
    m_eyeconOuter.setOutlineColor(sf::Color::Magenta);
    m_eyeconOuter.setOutlineThickness(2.f);
    m_eyeconOuter.setPosition(0.f, GameConst::RoomWidth / 2.f);

    //load shader
    m_shaders.preload(ShaderID::Sprite3DTextured, SpriteVertexLighting, SpriteFragmentTextured);
    m_shaders.get(ShaderID::Sprite3DTextured).setUniform("u_highlightColour", sf::Glsl::Vec3(1.f, 1.f, 1.f));

    //set up a background
    m_roomTexture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/rooms/editor.png");

    auto entity = m_uiScene.createEntity();
    entity.addComponent<xy::Transform>();
    entity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
    entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
    entity.getComponent<xy::Drawable>().setTexture(&m_roomTexture);
    entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
    entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
    entity.getComponent<xy::Drawable>().bindUniform("u_highlightColour", sf::Vector3f(1.f, 1.f, 1.f));
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
    if (!modelEntity.isValid())
    {
        if (m_defaultTexID == 0)
        {
            m_defaultTexID = m_resources.load<sf::Texture>("assets/images/cam_debug.png");
        }
        auto& tex = m_resources.get<sf::Texture>(m_defaultTexID);

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
        float range = zExtremes.second->Position.Z;
        if (zExtremes.first->Position.Z < 0)
        {
            range -= zExtremes.first->Position.Z;
        }

        modelEntity.getComponent<Sprite3D>().depth = range;

        //add minheight to all z vals to make sure model starts on ground..?
        for (auto i : meshIndices)
        {
            sf::Vertex vert;
            //positions
            vert.position.x = meshVerts[i].Position.X;
            vert.position.y = -meshVerts[i].Position.Y;

            float z = meshVerts[i].Position.Z;
            if (z < 0)
            {
                z -= zExtremes.first->Position.Z;
            }
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

        //check the material for a texture name
        if (!m_objLoader.LoadedMaterials.empty())
        {
            m_modelTextureName = m_objLoader.LoadedMaterials[0].map_Kd;
        }
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
            cfg.addProperty("texture").setValue(m_modelTextureName);
            cfg.addProperty("depth").setValue(modelEntity.getComponent<Sprite3D>().depth);
            cfg.save(path.substr(0, path.find_last_of('.')) + ".xmd");

            modelEntity.getComponent<xy::Drawable>().getVertices().clear();
            m_modelLoaded = false;
            m_modelTextureName.clear();

            return true;
        }
    }
    return false;
}

void ModelState::loadModel(const std::string& path)
{
    if (m_modelList.size() == MaxModels)
    {
        xy::Logger::log("Maximum models have already been loaded", xy::Logger::Type::Info);
        return;
    }

    auto absPath = path;
    if (xy::FileSystem::getFileExtension(path) == ".xym")
    {
        absPath.pop_back();
        absPath.pop_back();
        absPath += "md";
    }
    else if (xy::FileSystem::getFileExtension(path) != ".xmd")
    {
        absPath += ".xmd";
    }

    xy::ConfigFile cfg;
    cfg.loadFromFile(absPath);
    if (cfg.getName() == "model")
    {
        std::string modelName = xy::FileSystem::getFileName(absPath);
        modelName = modelName.substr(0, modelName.find_last_of('.'));

        parseModelNode(cfg, modelName);
    }
}

void ModelState::parseModelNode(const xy::ConfigObject& cfg, const std::string& modelName)
{
    std::string binPath;
    std::string texture;
    float depth = 0.f;
    sf::Vector2f position;
    float rotation = 0.f;

    const auto& properties = cfg.getProperties();
    for (const auto& prop : properties)
    {
        const auto name = prop.getName();
        if (name == "bin")
        {
            binPath = prop.getValue<std::string>();
        }
        else if (name == "texure")
        {
            texture = prop.getValue<std::string>();
        }
        else if (name == "depth")
        {
            depth = prop.getValue<float>();
        }
        else if (name == "position")
        {
            position = prop.getValue<sf::Vector2f>();
        }
        else if (name == "rotation")
        {
            rotation = prop.getValue<float>();
        }
    }

    if (!binPath.empty() && depth > 0)
    {
        //TODO load texture if name isn't empty
        if (m_defaultTexID == 0)
        {
            m_defaultTexID = m_resources.load<sf::Texture>("assets/images/cam_debug.png");
        }
        auto& tex = m_resources.get<sf::Texture>(m_defaultTexID);

        auto entity = m_uiScene.createEntity();
        entity.addComponent<xy::Transform>().setPosition(position);
        entity.getComponent<xy::Transform>().setRotation(rotation);
        entity.addComponent<NodeData>().binPath = binPath;
        entity.getComponent<NodeData>().texture = texture;
        entity.addComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
        entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);
        entity.getComponent<xy::Drawable>().setTexture(&tex);
        entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::Sprite3DTextured));
        entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
        entity.getComponent<xy::Drawable>().setCulled(false);
        entity.getComponent<xy::Drawable>().setPrimitiveType(sf::Triangles);
        entity.addComponent<Sprite3D>(m_matrixPool).depth = depth;
        entity.getComponent<xy::Drawable>().bindUniform("u_modelMat", &entity.getComponent<Sprite3D>().getMatrix()[0][0]);
        entity.getComponent<xy::Drawable>().bindUniform("u_highlightColour", sf::Vector3f(1.f, 1.f, 1.f));
        auto& verts = entity.getComponent<xy::Drawable>().getVertices();
        readModelBinary(verts, xy::FileSystem::getResourcePath() + binPath);
        entity.getComponent<xy::Drawable>().updateLocalBounds();

        if (m_modelList.count(modelName) != 0)
        {
            m_uiScene.destroyEntity(m_modelList[modelName]);
        }
        m_modelList[modelName] = entity;
    }
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
    m_eyeconOuter.setPosition(positions[direction] / 2.f);
}

void ModelState::setRoomGeometry()
{
    XY_ASSERT(m_sharedData.currentRoom > 0 && m_sharedData.currentRoom < m_roomList.size(), "Invalid Index");

    for (const auto& [name, ent] : m_modelList)
    {
        m_uiScene.destroyEntity(ent);
    }
    m_modelList.clear();
    m_selectedModel = {};
    m_aerialObjects.clear();
    m_aerialWalls.clear();

    auto* room = m_roomList[m_sharedData.currentRoom];
    const auto& properties = room->getProperties();
    
    m_wallFlags = 0;
    for (const auto& prop : properties)
    {
        //TODO look for texture path
        if (prop.getName() == "ceiling"
            && prop.getValue<std::int32_t>() != 0)
        {
            m_wallFlags |= WallFlags::Ceiling;
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
                        m_wallFlags |= WallFlags::North;
                        break;
                    case 1:
                        m_wallFlags |= WallFlags::East;
                        break;
                    case 2:
                        m_wallFlags |= WallFlags::South;
                        break;
                    case 3:
                        m_wallFlags |= WallFlags::West;
                        break;
                    }
                }
            }
        }

        //look for model objects
        else if (obj.getName() == "model")
        {
            parseModelNode(obj, obj.getId());
        }
    }

    auto& verts = m_roomEntity.getComponent<xy::Drawable>().getVertices();
    verts.clear();
    addFloor(verts);

    for (auto i = 0; i < 4; ++i)
    {
        if (m_wallFlags & (1 << i))
        {
            addWall(verts, i);
        }
    }
    if (m_wallFlags & WallFlags::Ceiling)
    {
        addCeiling(verts);
    }

    m_roomEntity.getComponent<xy::Drawable>().updateLocalBounds();

    //aerial view
    if (m_wallFlags & WallFlags::North)
    {
        auto& r = m_aerialWalls.emplace_back();
        r.setFillColor(sf::Color::Red);
        r.setSize({ GameConst::RoomWidth, 4.f });
        r.setOrigin(r.getSize() / 2.f);
        r.setPosition(0.f, -GameConst::RoomWidth / 2.f);
    }
    if (m_wallFlags & WallFlags::East)
    {
        auto& r = m_aerialWalls.emplace_back();
        r.setFillColor(sf::Color::Red);
        r.setSize({ 4.f, GameConst::RoomWidth });
        r.setOrigin(r.getSize() / 2.f);
        r.setPosition(GameConst::RoomWidth / 2.f, 0.f);
    }
    if (m_wallFlags & WallFlags::South)
    {
        auto& r = m_aerialWalls.emplace_back();
        r.setFillColor(sf::Color::Red);
        r.setSize({ GameConst::RoomWidth, 4.f });
        r.setOrigin(r.getSize() / 2.f);
        r.setPosition(0.f, GameConst::RoomWidth / 2.f);
    }
    if (m_wallFlags & WallFlags::West)
    {
        auto& r = m_aerialWalls.emplace_back();
        r.setFillColor(sf::Color::Red);
        r.setSize({ 4.f, GameConst::RoomWidth });
        r.setOrigin(r.getSize() / 2.f);
        r.setPosition(-GameConst::RoomWidth / 2.f, 0.f);
    }
}

void ModelState::saveRoom()
{
    //insert any model nodes
    auto* room = m_roomList[m_sharedData.currentRoom];
    for (const auto& [name, entity] : m_modelList)
    {
        if (auto* modelNode = room->findObjectWithId(name); !modelNode || modelNode->getName() != "model")
        {
            modelNode = room->addObject("model", name);
            modelNode->addProperty("bin").setValue(entity.getComponent<NodeData>().binPath);
            modelNode->addProperty("texture").setValue(entity.getComponent<NodeData>().texture);
            modelNode->addProperty("depth").setValue(entity.getComponent<Sprite3D>().depth);
            modelNode->addProperty("position").setValue(entity.getComponent<xy::Transform>().getPosition());
            modelNode->addProperty("rotation").setValue(entity.getComponent<xy::Transform>().getRotation());
        }
        else
        {
            if (auto* binProp = modelNode->findProperty("bin"); !binProp)
            {
                modelNode->addProperty("bin").setValue(entity.getComponent<NodeData>().binPath);
            }
            else
            {
                binProp->setValue(entity.getComponent<NodeData>().binPath);
            }

            if (auto* texProp = modelNode->findProperty("texture"); !texProp)
            {
                modelNode->addProperty("texture").setValue(entity.getComponent<NodeData>().texture);
            }
            else
            {
                texProp->setValue(entity.getComponent<NodeData>().texture);
            }

            if (auto* depthProp = modelNode->findProperty("depth"); !depthProp)
            {
                modelNode->addProperty("depth").setValue(entity.getComponent<Sprite3D>().depth);
            }
            else
            {
                depthProp->setValue(entity.getComponent<Sprite3D>().depth);
            }

            if (auto* posProp = modelNode->findProperty("position"); !posProp)
            {
                modelNode->addProperty("position").setValue(entity.getComponent<xy::Transform>().getPosition());
            }
            else
            {
                posProp->setValue(entity.getComponent<xy::Transform>().getPosition());
            }

            if (auto* rotProp = modelNode->findProperty("rotation"); !rotProp)
            {
                modelNode->addProperty("rotation").setValue(entity.getComponent<xy::Transform>().getRotation());
            }
            else
            {
                rotProp->setValue(entity.getComponent<xy::Transform>().getRotation());
            }
        }
    }

    m_mapData.save(xy::FileSystem::getResourcePath() + "assets/game.map");
}