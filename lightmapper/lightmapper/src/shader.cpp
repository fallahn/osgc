#include "shader.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

Shader::Shader()
{

}

Shader::~Shader()
{
    deleteProgram();
}

//public
void Shader::load(const char* vp, const char* fp, const char** attributes, int attributeCount)
{
    deleteProgram();

    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vp);
    if (!vertexShader)
    {
        return;
    }
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fp);
    if (!fragmentShader)
    {
        glDeleteShader(vertexShader);
        return;
    }

    GLuint program = glCreateProgram();
    if (program == 0)
    {
        fprintf(stderr, "Could not create program!\n");
        return;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    for (int i = 0; i < attributeCount; i++)
        glBindAttribLocation(program, i, attributes[i]);

    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        fprintf(stderr, "Could not link program!\n");
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen)
        {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            fprintf(stderr, "%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(program);
        return;
    }

    programID = program;

    //not all these necessarily exist - it'll do for now though
    modelUniform = glGetUniformLocation(programID, "u_model");
    viewUniform = glGetUniformLocation(programID, "u_view");
    projectionUniform = glGetUniformLocation(programID, "u_projection");
    textureUniform = glGetUniformLocation(programID, "u_texture");
    colourUniform = glGetUniformLocation(programID, "u_colour");
}

//private
GLuint Shader::loadShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0)
    {
        fprintf(stderr, "Could not create shader!\n");
        return 0;
    }
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        fprintf(stderr, "Could not compile shader!\n");
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen)
        {
            char* infoLog = (char*)malloc(infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            fprintf(stderr, "%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void Shader::deleteProgram()
{
    if (programID)
    {
        glDeleteProgram(programID);
    }

    programID = 0;
    textureUniform = 0;
    projectionUniform = 0;
    viewUniform = 0;
    modelUniform = 0;
}