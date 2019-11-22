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
    : m_window          (nullptr),
    m_initOK            (false),
    m_projectionMatrix  (1.f),
    m_viewMatrix        (1.f)
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
    ui::mapBrowserWindow();

    calcViewMatrix();
}

void App::calcViewMatrix()
{
    // initial camera config
    //TODO these should be members
    static glm::vec3 position(0.f, 0.3f, 1.5f);
    static glm::vec2 rotation(0.f);

    glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), rotation.y * static_cast<float>(M_PI / 180.f), glm::vec3(0.f, 1.f, 0.f));
    rotMat = glm::rotate(rotMat, rotation.x * static_cast<float>(M_PI / 180.f), glm::vec3(1.f, 0.f, 0.f));


    //this should probably be in the update or handle events func...
    if (!ImGui::GetIO().WantCaptureKeyboard
        && !ImGui::GetIO().WantCaptureMouse)
    {
        // mouse look
        static double lastMouse[] = { 0.0, 0.0 };
        double mouse[2];
        glfwGetCursorPos(m_window, &mouse[0], &mouse[1]);
        if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            rotation[0] += (float)(mouse[1] - lastMouse[1]) * -0.2f;
            rotation[1] += (float)(mouse[0] - lastMouse[0]) * -0.2f;
        }
        lastMouse[0] = mouse[0];
        lastMouse[1] = mouse[1];

        // keyboard movement (WSADEQ)
        float speed = (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 0.1f : 0.01f;
        glm::vec3 movement(0.f);
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) movement[2] -= speed;
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) movement[2] += speed;
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) movement[0] -= speed;
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) movement[0] += speed;
        if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) movement[1] -= speed;
        if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) movement[1] += speed;

        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(m_window, GLFW_TRUE);

        position += glm::vec3(rotMat * glm::vec4(movement, 1.f));
    }

    //construct view matrix
    m_viewMatrix = glm::inverse(glm::translate(glm::mat4(1.f), position) * rotMat);
}

void App::draw()
{
    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    glViewport(0, 0, w, h);


    //draw to screen with a blueish sky
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_scene.draw(m_viewMatrix, m_projectionMatrix);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}