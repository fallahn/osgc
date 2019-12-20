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

#include "EditorState.hpp"
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "ResourceIDs.hpp"
#include "Camera3D.hpp"

#include "glm/gtc/matrix_transform.hpp"

#include <xyginext/gui/Gui.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>

#include <xyginext/ecs/systems/CameraSystem.hpp>
#include <xyginext/ecs/systems/SpriteSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>

#include <SFML/OpenGL.hpp>
#include <SFML/Window/Event.hpp>

namespace
{
#include "tilemap.inl"
#include "3dRender.inl"

    const std::array<float, 5> ZoomLevels =
    {
        xy::DefaultSceneSize.y / 4.f,
        xy::DefaultSceneSize.y / 2.f,
        xy::DefaultSceneSize.y,
        xy::DefaultSceneSize.y * 2.f,
        xy::DefaultSceneSize.y * 4.f
    };

    const std::array<float, 5> ZoomAmount =
    {
        0.25f, 0.5f, 1.f, 2.f, 4.f
    };

    const float NearPlane = 700.f;
    const float FarPlane = 1900.f;

    const float DragSpeed = 2.5f;
}

EditorState::EditorState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    :xy::State  (ss, ctx),
    m_sharedData(sd),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_currentMap("None"),
    m_currentZoom(2)
{
    launchLoadingScreen();
    initScene();
    loadResources();
    initUI();
    quitLoadingScreen();
}

//public
bool EditorState::handleEvent(const sf::Event& evt)
{
    if (xy::ui::wantsKeyboard() || xy::ui::wantsMouse())
    {
        return false;
    }

    if (evt.type == sf::Event::MouseWheelScrolled)
    {
        if (evt.mouseWheelScroll.delta > 0)
        {
            zoom(true, evt.mouseWheelScroll.x, evt.mouseWheelScroll.y);
        }
        else
        {
            zoom(false, evt.mouseWheelScroll.x, evt.mouseWheelScroll.y);
        }
    }
    else if (evt.type == sf::Event::MouseMoved)
    {
        auto lastPos = m_mousePos;
        m_mousePos = sf::Vector2f(evt.mouseMove.x, evt.mouseMove.y);

        if (sf::Mouse::isButtonPressed(sf::Mouse::Middle))
        {
            float ratio = xy::DefaultSceneSize.x / static_cast<float>(getContext().renderWindow.getSize().x);
            m_scene.getActiveCamera().getComponent<xy::Transform>().move((lastPos - m_mousePos) * ZoomAmount[m_currentZoom] * ratio);
        }

        //TODO check if we're dragging an object
    }

    m_scene.forwardEvent(evt);

    return false;
}

void EditorState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool EditorState::update(float dt)
{
    const auto& mat = m_scene.getActiveCamera().getComponent<Camera3D>().viewProjectionMatrix;
    m_shaders.get(ShaderID::TileMap3D).setUniform("u_viewProjectionMatrix", sf::Glsl::Mat4(&mat[0][0]));
    m_shaders.get(ShaderID::TileEdge).setUniform("u_viewProjectionMatrix", sf::Glsl::Mat4(&mat[0][0]));
    //m_shaders.get(ShaderID::Sprite3D).setUniform("u_viewProjectionMatrix", sf::Glsl::Mat4(&mat[0][0]));
    //m_shaders.get(ShaderID::SpriteDepth).setUniform("u_viewProjectionMatrix", sf::Glsl::Mat4(&mat[0][0]));

    m_scene.update(dt);
    return false;
}

void EditorState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void EditorState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();

    m_scene.addSystem<xy::SpriteSystem>(mb);
    m_scene.addSystem<Camera3DSystem>(mb);
    m_scene.addSystem<xy::CameraSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(xy::DefaultSceneSize);
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(getContext().defaultView.getViewport());

    auto& cam3D = m_scene.getActiveCamera().addComponent<Camera3D>();
    auto fov = cam3D.calcFOV(xy::DefaultSceneSize.y);
    cam3D.projectionMatrix = glm::perspective(fov, xy::DefaultSceneSize.x / xy::DefaultSceneSize.y, NearPlane, FarPlane);
}

void EditorState::loadResources()
{
    m_shaders.preload(ShaderID::TileMap, tilemapFrag2, sf::Shader::Fragment);
    m_shaders.preload(ShaderID::TileMap3D, Layer3DVertex, tilemapFrag2);
    m_shaders.preload(ShaderID::TileEdge, TileEdgeVert, TileEdgeFrag);
}

void EditorState::initUI()
{
    registerWindow([&]()
        {
            static bool ShowMapProperties = true;
            static bool ShowObjectProperties = true;
        
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    ImGui::MenuItem("New", nullptr, nullptr);
                    ImGui::MenuItem("Open", nullptr, nullptr);
                    ImGui::MenuItem("Save", nullptr, nullptr);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    ImGui::MenuItem("Map Properties##0", nullptr, &ShowMapProperties);
                    ImGui::MenuItem("Object Inspector##0", nullptr, &ShowObjectProperties);

                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
        

            if (ShowMapProperties)
            {
                ImGui::SetNextWindowPos({ 20.f, 40.f }/*, ImGuiCond_FirstUseEver*/);
                ImGui::SetNextWindowSize({ 280.f, 120.f });
                if (ImGui::Begin("Map Properties##1", &ShowMapProperties))
                {
                    ImGui::Text("Map File: ");
                    ImGui::SameLine();
                    ImGui::Text(m_currentMap.c_str());
                    ImGui::SameLine();
                    if (ImGui::Button("Browse"))
                    {
                        auto path = xy::FileSystem::openFileDialogue("", "tmx");
                        if (!path.empty() && xy::FileSystem::getFileExtension(path) == ".tmx")
                        {
                            m_currentMap = path;
                            loadMap();
                        }
                    }

                    const char* items[] = { "Gearboy", "MES" }; //TODO read themes / set shared data
                    static const char* currentItem = nullptr;
                    ImGui::Text("Theme:    "); ImGui::SameLine();
                    if (ImGui::BeginCombo("##Theme", nullptr))
                    {
                        for (auto i = 0u; i < 2u; ++i)
                        {
                            bool selected = (currentItem == items[i]);
                            if (ImGui::Selectable(items[i], selected))
                            {
                                currentItem = items[i];
                            }

                            if (selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }

                        ImGui::EndCombo();
                    }

                    static char buff[10];
                    ImGui::Text("Next Map: "); ImGui::SameLine();
                    ImGui::InputText("##Next Map:", buff, 10);

                    ImGui::End();
                }
            }

            if (ShowObjectProperties)
            {
                ImGui::SetNextWindowPos({ 20.f, 180.f }/*, ImGuiCond_FirstUseEver*/);
                ImGui::SetNextWindowSize({ 280.f, 600.f });
                if (ImGui::Begin("Object Inspector", &ShowObjectProperties))
                {
                    ImGui::Text("Select an Object");
                    ImGui::End();
                }
            }
        });
}

void EditorState::loadMap()
{
    auto mapFile = xy::FileSystem::getFileName(m_currentMap);

    if (m_mapLoader.load(mapFile))
    {
        for (auto e : m_mapEntities)
        {
            m_scene.destroyEntity(e);
        }
        m_mapEntities.clear();


        const float scale = GameConst::PixelsPerTile / m_mapLoader.getTileSize();

        //map layers
        const auto& layers = m_mapLoader.getLayers();

        //for each layer create a drawable in the scene
        std::int32_t startDepth = GameConst::Depth::LaterStart;
        float worldDepth = 32.f;
        for (const auto& layer : layers)
        {
            //create the edge geometry
            const auto& edgeData = layer.edges;
            if (!edgeData.empty())
            {
                auto entity = m_scene.createEntity();
                //there's no model matrix (atm) so scale is applied directly to vertices.
                entity.addComponent<xy::Transform>();
                entity.addComponent<xy::Drawable>().setDepth(GameConst::Depth::LayerEdge); //so transparent objects draw on top correctly
                auto& verts = entity.getComponent<xy::Drawable>().getVertices();
                for (const auto& edge : edgeData)
                {
                    sf::Vector2f offset;

                    switch (edge.facing)
                    {
                    default: break;
                    case 0:
                        offset.y += m_mapLoader.getTileSize();
                        break;
                    case 1:
                        offset.x -= m_mapLoader.getTileSize();
                        break;
                    case 2:
                        offset.y -= m_mapLoader.getTileSize();
                        break;
                    case 3:
                        offset.x = m_mapLoader.getTileSize();
                        break;
                    }

                    verts.push_back(edge.vertices[1]);
                    verts.back().color.a = 128 - 32;
                    verts.back().position *= scale;
                    verts.back().texCoords += offset;

                    verts.push_back(edge.vertices[0]);
                    verts.back().color.a = 128 - 32;
                    verts.back().position *= scale;
                    verts.back().texCoords += offset;

                    verts.push_back(edge.vertices[0]);
                    verts.back().color.a = 128 + worldDepth;
                    verts.back().position *= scale;

                    verts.push_back(edge.vertices[1]);
                    verts.back().color.a = 128 + worldDepth;
                    verts.back().position *= scale;
                }
                entity.getComponent<xy::Drawable>().updateLocalBounds();
                entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::TileEdge));
                entity.getComponent<xy::Drawable>().setTexture(layer.tileSet);
                entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_texture");
                entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
                entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);

                m_mapEntities.push_back(entity);
            }


            auto entity = m_scene.createEntity();
            //entity = m_tilemapScene.createEntity();
            entity.addComponent<xy::Transform>();
            entity.addComponent<xy::Drawable>().setDepth(startDepth);
            entity.getComponent<xy::Drawable>().setTexture(layer.indexMap);
            entity.getComponent<xy::Drawable>().setShader(&m_shaders.get(ShaderID::TileMap3D));
            entity.getComponent<xy::Drawable>().bindUniformToCurrentTexture("u_indexMap");
            entity.getComponent<xy::Drawable>().bindUniform("u_tileSet", *layer.tileSet);
            entity.getComponent<xy::Drawable>().bindUniform("u_indexSize", sf::Vector2f(layer.indexMap->getSize()));
            entity.getComponent<xy::Drawable>().bindUniform("u_tileCount", layer.tileSetSize);
            entity.getComponent<xy::Drawable>().bindUniform("u_tileSize", sf::Vector2f(m_mapLoader.getTileSize(), m_mapLoader.getTileSize()));

            entity.getComponent<xy::Drawable>().bindUniform("u_depth", edgeData.empty() ? 0.f : worldDepth);
            entity.getComponent<xy::Drawable>().addGlFlag(GL_DEPTH_TEST);
            entity.getComponent<xy::Drawable>().addGlFlag(GL_CULL_FACE);

            auto& layerVerts = entity.getComponent<xy::Drawable>().getVertices();
            sf::Vector2f texCoords(layer.indexMap->getSize());

            layerVerts.emplace_back(sf::Vector2f(), sf::Vector2f());
            layerVerts.emplace_back(sf::Vector2f(layer.layerSize.x, 0.f), sf::Vector2f(texCoords.x, 0.f));
            layerVerts.emplace_back(layer.layerSize, texCoords);
            layerVerts.emplace_back(sf::Vector2f(0.f, layer.layerSize.y), sf::Vector2f(0.f, texCoords.y));
            std::reverse(layerVerts.begin(), layerVerts.end());

            entity.getComponent<xy::Drawable>().updateLocalBounds();
            entity.getComponent<xy::Transform>().setScale(scale, scale);

            startDepth += 1;

            m_mapEntities.push_back(entity);
        }


        m_sharedData.nextMap = mapFile;
        m_currentMap = mapFile;
    }
    else
    {
        m_currentMap = "Failed to open map.";
    }
}

void EditorState::zoom(bool in, int x, int y)
{
    if (in)
    {
        if (m_currentZoom > 0)
        {
            m_currentZoom--;
        }
    }
    else
    {
        m_currentZoom = std::min(m_currentZoom + 1, ZoomLevels.size() - 1);
    }

    auto& cam = m_scene.getActiveCamera().getComponent<Camera3D>();
    auto fov = cam.calcFOV(ZoomLevels[m_currentZoom]);
    cam.projectionMatrix = glm::perspective(fov, xy::DefaultSceneSize.x / xy::DefaultSceneSize.y, NearPlane, FarPlane);
}