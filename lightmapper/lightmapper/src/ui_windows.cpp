#include "App.hpp"
#include "tinyfiledialogs.h"
#include "imgui/imgui.h"
#include "geometry.h"

#include <iostream>
#include <string>

void App::mapBrowserWindow()
{
    static bool openFile = false;
    static bool bakeSelected = false;

    ImGui::SetNextWindowSize({ 200.f, 400.f }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Map Browser", nullptr, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::MenuItem("Open", nullptr, &openFile);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Tools"))
            {
                ImGui::MenuItem("Bake Selected", nullptr, &bakeSelected);
                ImGui::MenuItem("Bake All", nullptr, &m_bakeAll);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
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
                        updateGeometry(room, m_scene);
                        m_clearColour = room.skyColour;

                        m_currentRoom = room.id;
                        //TODO load room props
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
    }

    static const char* filters[] = { "*.map" };

    if (openFile)
    {
        auto path = tinyfd_openFileDialog("Open Map", nullptr, 1, filters, nullptr, 0);
        if (path)
        {
            loadMapData(path);
        }

        openFile = false;
    }

    if (bakeSelected)
    {
        if (m_currentRoom != -1)
        {
            m_scene.bake(std::to_string(m_mapData[m_currentRoom].id), m_clearColour);
        }
        bakeSelected = false;
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

        if (ImGui::Button("Bake")
            && m_currentRoom != -1)
        {
            m_scene.bake(std::to_string(m_mapData[m_currentRoom].id), m_clearColour);
        }

        ImGui::End();
    }
}