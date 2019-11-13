#pragma once

#define _USE_MATH_DEFINES
#ifndef M_PI // even with _USE_MATH_DEFINES not always available
#define M_PI 3.14159265358979323846
#endif

#include <glad/glad.h>

typedef struct
{
    float p[3];
    float t[2];
} vertex_t;

typedef struct
{
    GLuint program;
    GLint u_lightmap;
    GLint u_projection;
    GLint u_view;

    GLuint lightmap;
    int w, h;

    GLuint vao, vbo, ibo;
    vertex_t* vertices;
    unsigned short* indices;
    unsigned int vertexCount, indexCount;
} scene_t;