#include "App.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "gl_helpers.h"
#include "geometry.h"
#include "ConfigFile.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <iostream>

namespace
{
    //urg.
    double scrollOffset = 0.f;

    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        scrollOffset = yoffset;
    }
}

App::App()
    : m_window          (nullptr),
    m_initOK            (false),
    m_projectionMatrix  (1.f),
    m_viewMatrix        (1.f),
    m_bakeAll           (false)
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
        //TODO need much more file validating on this
        iFile.read((char*)&w, sizeof(w));
        iFile.read((char*)&h, sizeof(h));

        std::size_t size = 0;
        iFile.read((char*)&size, sizeof(size));

        if (size > 0)
        {
            m_outputPath.resize(size);
            iFile.read(m_outputPath.data(), size);
        }

        iFile.close();
    }


    m_window = glfwCreateWindow(w, h, "Lightmapper", NULL, NULL);
    if (!m_window)
    {
        std::cout << "Could not create window.\n";
        glfwTerminate();
    }
    else
    {
        glfwSetScrollCallback(m_window, scroll_callback);

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

        if (!m_outputPath.empty())
        {
            std::size_t size = m_outputPath.size();
            oFile.write((char*)&size, sizeof(size));
            oFile.write(m_outputPath.c_str(), size);
        }
        else
        {
            std::size_t size = 0;
            oFile.write((char*)&size, sizeof(size));
        }
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

        if (m_bakeAll)
        {
            bakeAll();
            m_bakeAll = false;
        }
    }
}

//private
void App::loadMapData(const std::string& path)
{
    xy::ConfigFile cfg;
    if (cfg.loadFromFile(path))
    {
        m_mapData.clear();

        const auto& objects = cfg.getObjects();
        for (const auto& obj : objects)
        {
            if (obj.getName() == "room")
            {
                auto& roomData = m_mapData.emplace_back();
                try
                {
                    roomData.id = std::atoi(obj.getId().c_str());
                }
                catch (...)
                {
                    //invalid room
                    m_mapData.pop_back();
                    continue;
                }

                //TODO do we need to get the texture property?
                //probably not

                const auto& roomProperties = obj.getProperties();
                for (const auto& prop : roomProperties)
                {
                    if (prop.getName() == "ceiling")
                    {
                        if (prop.getValue<std::int32_t>() == 1)
                        {
                            roomData.flags |= RoomData::Flags::Ceiling;
                        }
                    }
                    else if (prop.getName() == "sky_colour")
                    {
                        //roomData.skyColour = prop.getValue<std::array<float, 3>>();
                        //this breaks the join between bakes so we'll leave it white
                        //and let the in-game lighting do its thing
                    }
                    else if (prop.getName() == "room_colour")
                    {
                        roomData.roomColour = prop.getValue<std::array<float, 3>>();
                    }
                }

                const auto& roomObjects = obj.getObjects();
                for (const auto& roomObj : roomObjects)
                {
                    if (roomObj.getName() == "wall")
                    {
                        const auto& properties = roomObj.getProperties();
                        for (const auto& prop : properties)
                        {
                            //we only need the wall direction property
                            if (prop.getName() == "direction")
                            {
                                roomData.flags |= (1 << prop.getValue<std::int32_t>());
                            }
                        }
                    }
                    else if (roomObj.getName() == "model")
                    {
                        auto& modelData = roomData.models.emplace_back();

                        //save the info and process geometry after the room is built
                        const auto& properties = roomObj.getProperties();
                        for (const auto& prop : properties)
                        {
                            if (prop.getName() == "bin")
                            {
                                //this assumes the models are stored next to the map
                                auto fullPath = path;
                                std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

                                for (auto i = 0; i < 2; ++i)
                                {
                                    if (auto pos = fullPath.find_last_of('/'); pos != std::string::npos)
                                    {
                                        fullPath = fullPath.substr(0, pos);
                                    }
                                }
                                modelData.path = fullPath + "/" + prop.getValue<std::string>();

                            }
                            else if (prop.getName() == "position")
                            {
                                modelData.position = prop.getValue<glm::vec2>();
                            }
                            else if (prop.getName() == "rotation")
                            {
                                modelData.rotation = prop.getValue<float>();
                            }
                            else if (prop.getName() == "depth")
                            {
                                modelData.depth = prop.getValue<float>();
                            }
                        }

                        if (modelData.path.empty() ||
                            modelData.depth <= 0)
                        {
                            //invalid data, remove from the list
                            roomData.models.pop_back();
                        }
                    }
                }

                /*if (roomData.flags & RoomData::Ceiling)
                {
                    roomData.skyColour[0] *= 0.1f;
                    roomData.skyColour[1] *= 0.1f;
                    roomData.skyColour[2] *= 0.1f;
                }*/
            }
        }

        std::sort(m_mapData.begin(), m_mapData.end(),
            [](const RoomData& a, const RoomData& b)
            {
                return a.id < b.id;
            });
    }
}

void App::handleEvents()
{
    glfwPollEvents();

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
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
    statusWindow();

    calcViewMatrix();
}

void App::calcViewMatrix()
{
    // initial camera config
    //TODO these should be members
    static glm::vec3 position(0.f, 0.3f, 2.5f);
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
        /*glm::vec3 movement(0.f);
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) movement[2] -= speed;
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) movement[2] += speed;
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) movement[0] -= speed;
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) movement[0] += speed;
        if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) movement[1] -= speed;
        if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) movement[1] += speed;*/
        /*position += glm::vec3(rotMat * glm::vec4(movement, 1.f));*/

        float speed = (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 0.85f : 0.5f;
        //TODO scale this based on current distance so that scrolling feels linear
        position.z = std::max(0.f, position.z - (static_cast<float>(scrollOffset) * speed));
        scrollOffset = 0.f;


    }

    //construct view matrix
    m_viewMatrix = glm::inverse(rotMat * glm::translate(glm::mat4(1.f), position)/* * rotMat*/);
}

void App::draw()
{
    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    glViewport(0, 0, w, h);


    //draw to screen with a blueish sky
    glClearColor(m_clearColour[0], m_clearColour[1], m_clearColour[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_scene.draw(m_viewMatrix, m_projectionMatrix);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}

void App::bakeAll()
{
    //hmmmm this really needs to be multithreaded but shared contexts blaarg

    for (const auto& room : m_mapData)
    {
        m_currentRoom = room.id;
        updateSceneGeometry(room);
        handleEvents();
        update();

        m_scene.bake(m_outputPath + std::to_string(room.id), room.skyColour);
        m_clearColour = room.skyColour;
        draw();
    }
}