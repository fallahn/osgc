#include "ui_windows.h"
#include "tinyfiledialogs.h"
#include "imgui/imgui.h"

#include <iostream>

void mapBrowserWindow()
{
    static bool openFile = false;

    ImGui::SetNextWindowSize({ 200.f, 400.f }, ImGuiCond_FirstUseEver);
    ImGui::Begin("Map Browser", nullptr, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Open", NULL, &openFile);
            ImGui::EndMenu();
        }
        /*if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Bake All", NULL, &show_app_metrics);
            ImGui::EndMenu();
        }*/
        ImGui::EndMenuBar();
    }

    ImGui::End();


    static const char* filters[] = { "*.map" };

    if (openFile)
    {
        auto path = tinyfd_openFileDialog("Open Map", nullptr, 1, filters, nullptr, 0);
        if (path)
        {
            std::cout << "opened a file " << path << "\n";
        }

        openFile = false; //why do we have to manually reset this?
    }
}