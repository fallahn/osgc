#include "App.hpp"
#include "tinyfiledialogs.h"
#include "imgui/imgui.h"
#include "geometry.h"

#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>

namespace
{
    static void HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

void App::mapBrowserWindow()
{
    static const char* mapFilters[] = { "*.map" };
    static const char* modelFilters[] = { "*.xmd" };
    static const char* objFilters[] = { "*.obj" };

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Map"))
            {
                auto path = tinyfd_openFileDialog("Open Map", m_lastPaths.lastMap.c_str(), 1, mapFilters, nullptr, 0);
                if (path)
                {
                    loadMapData(path);
                    m_lastPaths.lastMap = path;
                }
            }
            if (ImGui::MenuItem("Open Model"))
            {
                auto path = tinyfd_openFileDialog("Open Model", m_lastPaths.lastModel.c_str(), 1, modelFilters, nullptr, 0);
                if (path)
                {
                    loadModel(path);
                    m_lastPaths.lastModel = path;
                }
            }
            if (ImGui::MenuItem("Import OBJ"))
            {
                auto path = tinyfd_openFileDialog("Import obj", m_lastPaths.lastImport.c_str(), 1, objFilters, nullptr, 0);
                if (path)
                {
                    importObjFile(path);
                    m_lastPaths.lastImport = path;
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Bake Selected"))
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
            }
            ImGui::MenuItem("Bake All", nullptr, &m_bakeAll);
            if (ImGui::MenuItem("Set Output Directory"))
            {
                auto path = tinyfd_selectFolderDialog("Select Output Directory", m_outputPath.c_str());
                if (path)
                {
                    m_outputPath = path;
                    std::replace(m_outputPath.begin(), m_outputPath.end(), '\\', '/');
                    m_outputPath += '/';
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options"))
        {
            if (ImGui::MenuItem("Set Ground Texture"))
            {
                static const char* filter[] = { "*.png", "*.jpg", "*.tga", "*.bmp" };
                auto path = tinyfd_openFileDialog("Open Image", m_lastPaths.measurePath.c_str(), 1, filter, nullptr, 0);
                if (path)
                {
                    m_scene.createMeasureMesh(path);
                    m_lastPaths.measurePath = path;
                }
            }
            if (ImGui::MenuItem("Hitbox Mode", nullptr, &m_hitboxMode))
            {

            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGui::SetNextWindowSize({ 200.f, 400.f }, ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos({ 80.f, 20.f }, ImGuiCond_FirstUseEver);
    if (m_mapLoaded && ImGui::Begin("Map Browser", nullptr/*, ImGuiWindowFlags_MenuBar*/))
    {
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

}

void App::statusWindow()
{
    ImGui::SetNextWindowSize({ 200.f, 400.f }, ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos({ 580.f, 20.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Status"))
    {
        if (m_mapLoaded)
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
        }
        else
        {
            const char* items[] = { "yUp", "zUp" };
            static const char* selectedIndex = items[1];
            ImGui::PushItemWidth(60.f);
            if (ImGui::BeginCombo("Game World Orientation", selectedIndex))
            {
                for (auto i = 0u; i < 2; ++i)
                {
                    bool selected = selectedIndex == items[i];
                    if (ImGui::Selectable(items[i], selected))
                    {
                        selectedIndex = items[i];
                        setZUp(i == 1);
                        m_orthoPosition = glm::vec3(0.f);
                    }

                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            HelpMarker("Set this to the orientation of the game world. For example Bob is z-up but the platformer is y-up.\nThis will cause models to be displayed as they would in game.");
        }

        ImGui::Text("%s", m_scene.getProgress().c_str());

        if (!m_scene.getMeshes().empty())
        {
            ImGui::Image((void*)(intptr_t)m_scene.getMeshes()[0]->texture, { 480.f, 480.f });

            if (ImGui::Button("Load Texture"))
            {
                static const char* filter[] = { "*.png", "*.jpg", "*.tga", "*.bmp" };
                auto path = tinyfd_openFileDialog("Load Texture", m_lastPaths.lastTexture.c_str(), 4, filter, nullptr, 0);
                if (path)
                {
                    m_scene.getMeshes()[0]->loadTexture(path);
                    m_scene.getMeshes()[0]->setTextureSmooth(m_smoothTextures);
                    m_lastPaths.lastTexture = path;
                }
            }

            ImGui::SameLine();
            if (ImGui::Checkbox("Smooth Texture", &m_smoothTextures))
            {
                m_scene.getMeshes()[0]->setTextureSmooth(m_smoothTextures);
            }
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
        ImGui::SameLine();
        HelpMarker("Images will automatically be saved on completion if this is checked.");

        if (!m_scene.getMeshes().empty() &&
            ImGui::Button("Export Last Bake"))
        {
            const char* filter[] = { "*.png" };
            auto path = tinyfd_saveFileDialog("Save Texture", m_lastPaths.lastTexture.c_str(), 1, filter, nullptr);
            if (path)
            {
                std::string outpath = path;
                if (outpath.find(".png") == std::string::npos)
                {
                    outpath += ".png";
                }
                m_scene.saveLightmap(outpath);
                m_lastPaths.lastTexture = path;
            }
        }

        //import/export settings
        if (m_showImportWindow)
        {
            ImGui::Separator();
            ImGui::Text("%s", "Export Settings");
            ImGui::SameLine();
            HelpMarker("Adjust the transform to rotate and scale the model until it sits correctly on the measurement plane.\nMake sure world orientation (above) is set correctly, and that Apply Transform is checked to save changes when exporting the model");

            auto updateTransform = [&]()
            {
                static const float toRad = static_cast<float>(M_PI) / 180.f;

                auto& mesh = m_scene.getMeshes()[0];
                glm::vec3 zAxis = m_scene.getzUp() ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(0.f, 0.f, -1.f);
                mesh->modelMatrix = glm::rotate(glm::mat4(1.f), m_importTransform.rotation.y * toRad, zAxis);
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
            static bool binaryOnly = true;

            ImGui::Checkbox("Export Texture", &exportTexture);
            ImGui::SameLine();
            HelpMarker("Also export the baked texture when exporting the model");
            ImGui::SameLine();
            ImGui::Checkbox("Apply Transform on Export", &applyTransform);
            ImGui::SameLine();
            HelpMarker("Make sure this is checked to save any changes to the model's transform when exporting");
            ImGui::SameLine();
            ImGui::Checkbox("Binary Only", &binaryOnly);
            ImGui::SameLine();
            HelpMarker("Only export the model data and don't overwrite the configuration file");
            if (ImGui::Button("Export"))
            {
                static const char* filter[] = { "*.xmd" };
                auto path = tinyfd_saveFileDialog("Export Model", m_lastPaths.lastExport.c_str(), 1, filter, nullptr);
                if (path)
                {
                    exportModel(path, applyTransform, exportTexture, binaryOnly);
                    m_lastPaths.lastExport = path;
                }
            }
        }

        ImGui::End();
    }
}

void App::debugWindow()
{
    ImGui::SetNextWindowSize({ 200.f, 100.f }, ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos({ 80.f, 20.f }, ImGuiCond_FirstUseEver);

    if (ImGui::Begin("debug"))
    {
        ImGui::Text("Mouse Pos: %3.3f, %3.3f, %3.3f", m_mousePosition.x, m_mousePosition.y, m_mousePosition.z);
        ImGui::Text("Ortho Pos: %3.3f, %3.3f, %3.3f", m_orthoPosition.x, m_orthoPosition.y, m_orthoPosition.z);
    }
    ImGui::End();
}

void App::hitboxWindow()
{
    if (m_hitboxMode)
    {
        ImGui::SetNextWindowSize({ 200.f, 100.f }, ImGuiCond_FirstUseEver);
        ImGui::SetWindowPos({ 80.f, 420.f }, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Hit Boxes"))
        {
            if (ImGui::Button("Add"))
            {
                //set mode to add box
            }

            ImGui::Text("Properties");
            ImGui::Text("Nothing Selected");
            //else show size/spinner

            //if selected show delete button
        }
        ImGui::End();
    }
}