#include "App.hpp"
#include "tinyfiledialogs.h"
#include "imgui/imgui.h"
#include "geometry.h"

#include <iostream>
#include <string>

void App::mapBrowserWindow()
{
    static bool openFile = false;

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
                ImGui::MenuItem("Bake Selected", nullptr, nullptr);
                ImGui::MenuItem("Bake All", nullptr, nullptr);
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
                        if (room.flags & RoomData::Ceiling)
                        {
                            m_clearColour[0] *= 0.1f;
                            m_clearColour[1] *= 0.1f;
                            m_clearColour[2] *= 0.1f;
                        }

                        m_currentRoom = room.id;
                        //TODO load room props
                    }
                    label = "Bake##" + roomID;
                    if (ImGui::Button(label.c_str())
                        && m_currentRoom != -1)
                    {
                        //TODO set the sky colour
                        m_scene.bake(roomID, room.skyColour);
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

        openFile = false; //why do we have to manually reset this?
    }
}