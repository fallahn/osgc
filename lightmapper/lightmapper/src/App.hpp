#pragma once

#include "scene.h"

#include <GLFW/glfw3.h>

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

    void handleEvents();
    void update();

    void draw();
};
