#pragma once

#include "structures.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Shader final
{
public:
    Shader();
    ~Shader();

    Shader(const Shader&) = delete;
    Shader(Shader&&) = delete;

    const Shader& operator = (const Shader&) = delete;
    const Shader& operator = (Shader&&) = delete;

    GLuint programID = 0;
    GLint textureUniform = 0;
    GLint projectionUniform = 0;
    GLint viewUniform = 0;
    GLint modelUniform = 0;

    void load(const char* vp, const char* fp, const char** attributes, int attributeCount);

private:
    GLuint loadShader(GLenum type, const char* source);

    void deleteProgram();
};


//GLuint loadProgram();