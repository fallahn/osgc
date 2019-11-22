#include "App.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "gl_helpers.h"
#include "geometry.h"
#include "ui_windows.h"

#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>

App::App()
    : m_window  (nullptr),
    m_initOK    (false)
{
    if (glfwInit())
    {
        glfwWindowHint(GLFW_RED_BITS, 8);
        glfwWindowHint(GLFW_GREEN_BITS, 8);
        glfwWindowHint(GLFW_BLUE_BITS, 8);
        glfwWindowHint(GLFW_ALPHA_BITS, 8);
        glfwWindowHint(GLFW_DEPTH_BITS, 32);
        glfwWindowHint(GLFW_STENCIL_BITS, GLFW_DONT_CARE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 4);
    }

    int w = 800;
    int h = 600;
    std::ifstream iFile("window", std::ios::binary);
    if (!iFile.fail())
    {
        iFile.read((char*)&w, sizeof(w));
        iFile.read((char*)&h, sizeof(h));
    }


    m_window = glfwCreateWindow(w, h, "Lightmapper", NULL, NULL);
    if (!m_window)
    {
        std::cout << "Could not create window.\n";
        glfwTerminate();
    }
    else
    {
        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);
        //built in loader breaks ImGui
        //gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        gladLoadGL();

        const char* glslVer = "#version 150";

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true))
        {
            std::cout << "failed glfw init\n";
        }
        if (!ImGui_ImplOpenGL3_Init(glslVer))
        {
            std::cout << "failed opengl init\n";
        }
    }

    if (!m_scene.init())
    {
        std::cout << "Could not initialize scene.\n";
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
    else
    {
        m_projectionMatrix = glm::perspective(45.f * static_cast<float>(M_PI / 180.f), static_cast<float>(w) / static_cast<float>(h), 0.1f, 100.f);
        m_initOK = true;
    }
}

App::~App()
{
    m_scene.destroy();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    //save the current window size
    int w = 800;
    int h = 600;
    glfwGetFramebufferSize(m_window, &w, &h);
    glViewport(0, 0, w, h);

    std::ofstream oFile("window", std::ios::binary);
    if (oFile.is_open() && oFile.good())
    {
        std::vector<char>buff(sizeof(int) * 2);
        std::memcpy(buff.data(), &w, sizeof(int));
        std::memcpy(buff.data() + sizeof(int), &h, sizeof(int));
        oFile.write(buff.data(), buff.size());
    }

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

//public
void App::run()
{
    if (!m_initOK)
    {
        return;
    }

    while (!glfwWindowShouldClose(m_window))
    {
        //TODO fixed timestep?
        handleEvents();
        update();
        draw();
    }
}

//private
void App::handleEvents()
{
    glfwPollEvents();
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        m_scene.bake();
    }
}

void App::update()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //static bool show_demo_window = true;
    //ImGui::ShowDemoWindow(&show_demo_window);

    //custom imgui rendering
    mapBrowserWindow();
}

void App::draw()
{
    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    glViewport(0, 0, w, h);

    //camera for glfw window
    glm::mat4 view = fpsCameraViewMatrix(m_window);

    //draw to screen with a blueish sky
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_scene.draw(view, m_projectionMatrix);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}