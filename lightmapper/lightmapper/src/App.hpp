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

    void handleEvents();
    void update();

    void calcViewMatrix();

    void draw();
};
