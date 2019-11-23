#pragma once

#include "scene.h"

#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>

class App final
{
public:
    App();
    ~App();

    App(const App&) = delete;
    App(App&&) = delete;
    App& operator = (const App&) = delete;
    App& operator = (App&&) = delete;

    void run();

private:
    Scene m_scene;
    GLFWwindow* m_window;
    bool m_initOK;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    std::array<float, 3> m_clearColour = { 0.6f, 0.8f, 1.0f };

    std::vector<RoomData> m_mapData;
    std::int32_t m_currentRoom = -1;
    void loadMapData(const std::string&);

    std::string m_outputPath;

    void handleEvents();
    void update();

    void calcViewMatrix();
    void draw();

    void bakeAll();
    bool m_bakeAll;

    //ui stuffs
    void mapBrowserWindow();
    void statusWindow();
};
