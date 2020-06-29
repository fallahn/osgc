#pragma once

#include "App.hpp"
#include "GLFW/glfw3.h"

#define APP_INSTANCE(x) *static_cast<App*>(x)

void error_callback(int, const char*);

void mouse_move_callback(GLFWwindow*, double, double);

void mouse_button_callback(GLFWwindow*, int, int, int);

void keypress_callback(GLFWwindow*, int, int, int, int);
