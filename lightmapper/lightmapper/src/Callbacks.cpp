#include "Callbacks.hpp"

#include <iostream>

void error_callback(int err, const char* msg)
{
    std::cout << "Error " << err << ": " << msg << std::endl;
}

void mouse_move_callback(GLFWwindow* window, double, double)
{
    auto& app = APP_INSTANCE(glfwGetWindowUserPointer(window));

    auto updateRect = [&](bool start)
    {
        auto rect = app.m_selectedRect;
        glm::vec2 position = glm::vec2(0.f);
        if (app.m_scene.getzUp())
        {
            position = { app.m_mousePosition.x, app.m_mousePosition.z };
        }
        else
        {
            position = { app.m_mousePosition.x, app.m_mousePosition.y };
        }
        if (start)
        {
            rect->start = position;
        }
        else
        {
            rect->end = position;
        }
        rect->updateVerts();
    };


    switch (app.m_mouseState)
    {
    default:
    case App::MouseState::None:

        break;
    case App::MouseState::HandleStart:
        //move the start position of the active rectangle and update
        updateRect(true);
        break;
    case App::MouseState::HandleEnd:
        updateRect(false);
        break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto& app = APP_INSTANCE(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        switch (button)
        {
        default: break;
        case GLFW_MOUSE_BUTTON_LEFT:
            if (app.m_hitboxMode)
            {
                switch (app.m_mouseState)
                {
                default:
                case App::MouseState::None:
                    //check position and see if we pick up a handle
                    //or select a new rectangle if none active
                    app.grabHandle();
                    break;
                case App::MouseState::HandleStart:
                case App::MouseState::HandleEnd:
                    app.m_mouseState = App::MouseState::None;
                    break;
                }
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:

            break;
        }
    }
}
