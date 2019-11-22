#include "gl_helpers.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

GLuint loadShader(GLenum type, const char* source)
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
GLuint loadProgram(const char* vp, const char* fp, const char** attributes, int attributeCount)
{
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vp);
    if (!vertexShader)
        return 0;
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fp);
    if (!fragmentShader)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program == 0)
    {
        fprintf(stderr, "Could not create program!\n");
        return 0;
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
        return 0;
    }
    return program;
}