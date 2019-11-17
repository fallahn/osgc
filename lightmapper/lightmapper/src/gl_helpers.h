#pragma once

#include "structures.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

GLuint loadShader(GLenum type, const char* source);
GLuint loadProgram(const char* vp, const char* fp, const char** attributes, int attributeCount);

glm::mat4 fpsCameraViewMatrix(GLFWwindow* window);