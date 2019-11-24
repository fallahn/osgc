#include "App.hpp"
#include "tinyfiledialogs.h"
#include "imgui/imgui.h"
#include "geometry.h"

#include <iostream>
#include <string>

void App::mapBrowserWindow()
{
    static bool openMap = false;
    static bool openModel = false;
    static bool openOutput = false;
    static bool bakeSelected = false;

    ImGui::SetNextWindowSize({ 200.f, 400.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Map Browser", nullptr, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("Open Map", nullptr, &openMap);
                ImGui::MenuItem("Open Model", nullptr, &openModel);
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

        if (!m_mapData.empty() && ImGui::CollapsingHeader("Room List"))
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
                        //m_clearColour = room.skyColour;

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

    if (bakeSelected)
    {
        if (m_currentRoom != -1)
        {
            m_scene.bake(m_outputPath + std::to_string(m_mapData[m_currentRoom].id), /*m_clearColour*/m_mapData[m_currentRoom].skyColour);
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

        if (ImGui::Button("Bake"))
        {
            if (m_currentRoom != -1)
            {
                m_scene.bake(m_outputPath + std::to_string(m_mapData[m_currentRoom].id), /*m_clearColour*/m_mapData[m_currentRoom].skyColour);
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

        ImGui::End();
    }
}