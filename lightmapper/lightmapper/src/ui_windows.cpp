#include "App.hpp"
#include "tinyfiledialogs.h"
#include "imgui/imgui.h"
#include "geometry.h"

#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <string>
#include <algorithm>

void App::mapBrowserWindow()
{
    static bool openMap = false;
    static bool openModel = false;
    static bool openOutput = false;
    static bool bakeSelected = false;
    static bool importObj = false;

    ImGui::SetNextWindowSize({ 200.f, 400.f }, ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos({ 80.f, 20.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("Open Map", nullptr, &openMap);
                ImGui::MenuItem("Open Model", nullptr, &openModel);
                ImGui::MenuItem("Import OBJ", nullptr, &importObj);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools"))
            {
                ImGui::MenuItem("Bake Selected", nullptr, &bakeSelected);
                ImGui::MenuItem("Bake All", nullptr, &m_bakeAll);
                ImGui::MenuItem("Set Output Directory", nullptr, &openOutput);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (!m_outputPath.empty() && !m_mapData.empty())
        {
            ImGui::Text("%s", "Output Path:");
            ImGui::Text("%s", m_outputPath.c_str());
        }

        if (!m_mapData.empty() && ImGui::CollapsingHeader("Rooms"))
        {
            for (const auto& room : m_mapData)
            {
                auto roomID = std::to_string(room.id);
                if (ImGui::TreeNode(roomID.c_str()))
                {
                    std::string label = "Load##" + roomID;
                    if (ImGui::Button(label.c_str())
                        && m_currentRoom != room.id)
                    {
                        updateSceneGeometry(room);

                        m_currentRoom = room.id;
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }

    static const char* mapFilters[] = { "*.map" };
    static const char* modelFilters[] = { "*.xmd" };
    static const char* objFilters[] = { "*.obj" };

    if (openMap)
    {
        auto path = tinyfd_openFileDialog("Open Map", nullptr, 1, mapFilters, nullptr, 0);
        if (path)
        {
            loadMapData(path);
        }

        openMap = false;
    }

    if (openModel)
    {
        auto path = tinyfd_openFileDialog("Open Map", nullptr, 1, modelFilters, nullptr, 0);
        if (path)
        {
            loadModel(path);
        }

        openModel = false;
    }

    if (importObj)
    {
        auto path = tinyfd_openFileDialog("Import obj", nullptr, 1, objFilters, nullptr, 0);
        if (path)
        {
            importObjFile(path);
        }
        importObj = false;
    }

    if (bakeSelected)
    {
        if (m_currentRoom != -1)
        {
            std::string outpath;
            if (m_saveOutput)
            {
                outpath = m_outputPath + std::to_string(m_mapData[m_currentRoom].id);
            }
            m_scene.setLightmapSize(ConstVal::RoomTextureWidth, ConstVal::RoomTextureHeight);
            m_scene.bake(outpath, m_mapData[m_currentRoom].skyColour);
        }
        else
        {
            //if model loaded bake it
            if (!m_scene.getMeshes().empty())
            {
                bakeModel();
            }
        }
        bakeSelected = false;
    }

    if (openOutput)
    {
        auto path = tinyfd_selectFolderDialog("Select Output Directory", m_outputPath.c_str());
        if (path)
        {
            m_outputPath = path;
            std::replace(m_outputPath.begin(), m_outputPath.end(), '\\', '/');
            m_outputPath += '/';
        }

        openOutput = false;
    }
}

void App::statusWindow()
{
    ImGui::SetNextWindowSize({ 200.f, 400.f }, ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos({ 580.f, 20.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Status"))
    {
        ImGui::Text("%s", "Current Room: ");
        ImGui::SameLine();
        if (m_currentRoom > -1)
        {
            ImGui::Text("%s", std::to_string(m_currentRoom).c_str());
        }
        else
        {
            ImGui::Text("%s", "None");
        }

        ImGui::Text("%s", m_scene.getProgress().c_str());

        if (!m_scene.getMeshes().empty())
        {
            ImGui::Image((void*)(intptr_t)m_scene.getMeshes()[0]->texture, { 480.f, 480.f });
        }

        if (ImGui::Button("Bake"))
        {
            if (m_currentRoom != -1)
            {
                std::string outpath;
                if (m_saveOutput)
                {
                    outpath = m_outputPath + std::to_string(m_mapData[m_currentRoom].id);
                }
                m_scene.setLightmapSize(ConstVal::RoomTextureWidth, ConstVal::RoomTextureHeight);
                m_scene.bake(outpath, m_mapData[m_currentRoom].skyColour);
            }
            else
            {
                //bake model if loaded
                if (!m_scene.getMeshes().empty())
                {
                    bakeModel();
                }
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Save Output", &m_saveOutput);

        if (!m_scene.getMeshes().empty() &&
            ImGui::Button("Export Last Bake"))
        {
            const char* filter[] = { "*.png" };
            auto path = tinyfd_saveFileDialog("Save Texture", nullptr, 1, filter, nullptr);
            if (path)
            {
                std::string outpath = path;
                if (outpath.find(".png") == std::string::npos)
                {
                    outpath += ".png";
                }
                m_scene.saveLightmap(outpath);
            }
        }

        //import/export settings
        if (m_showImportWindow)
        {
            ImGui::Separator();
            ImGui::Text("%s", "Export Settings");

            auto updateTransform = [&]()
            {
                static const float toRad = static_cast<float>(M_PI) / 180.f;

                auto& mesh = m_scene.getMeshes()[0];
                mesh->modelMatrix = glm::rotate(glm::mat4(1.f), m_importTransform.rotation.y * toRad, glm::vec3(0.f, 1.f, 0.f));
                mesh->modelMatrix = glm::rotate(mesh->modelMatrix, m_importTransform.rotation.x * toRad, glm::vec3(1.f, 0.f, 0.f));

                mesh->modelMatrix = glm::scale(mesh->modelMatrix, m_importTransform.scale);
            };

            if (ImGui::DragFloat2("Rotation", reinterpret_cast<float*>(&m_importTransform.rotation), 1.f, 0.f, 360.f))
            {
                updateTransform();
            }

            if (ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&m_importTransform.scale), 0.1f, 0.1f, 10.f))
            {
                updateTransform();
            }

            static bool applyTransform = true;
            static bool exportTexture = false;

            ImGui::Checkbox("Export Texture", &exportTexture);
            ImGui::SameLine();
            ImGui::Checkbox("Apply Transform on Export", &applyTransform);
            if (ImGui::Button("Export"))
            {
                static const char* filter[] = { "*.xmd" };
                auto path = tinyfd_saveFileDialog("Export Model", nullptr, 1, filter, nullptr);
                if (path)
                {
                    exportModel(path, applyTransform, exportTexture);
                }
            }
            
        }

        ImGui::End();
    }
}