#pragma once

#include "structures.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

GLuint loadShader(GLenum type, const char* source);
GLuint loadProgram(const char* vp, const char* fp, const char** attributes, int attributeCount);
void multiplyMatrices(float* out, float* a, float* b);
void translationMatrix(float* out, float x, float y, float z);
void rotationMatrix(float* out, float angle, float x, float y, float z);
void transformPosition(float* out, float* m, float* p);
void transposeMatrix(float* out, float* m);
void perspectiveMatrix(float* out, float fovy, float aspect, float zNear, float zFar);
void fpsCameraViewMatrix(GLFWwindow* window, float* view);