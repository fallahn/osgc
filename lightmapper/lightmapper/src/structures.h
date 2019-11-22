#pragma once

#define _USE_MATH_DEFINES
#ifndef M_PI // even with _USE_MATH_DEFINES not always available
#define M_PI 3.14159265358979323846
#endif

#include <glad/glad.h>
#include <glm/mat4x4.hpp>

#include <cstdint>
#include <vector>
#include <array>

struct Vertex final
{
    Vertex()
    {
        position[0] = 0.f;
        position[1] = 0.f;
        position[2] = 0.f;

        texCoord[0] = 0.f;
        texCoord[1] = 0.f;
    }
    float position[3];
    float texCoord[2];
};
using vertex_t = Vertex;

struct Mesh final
{
    Mesh();
    ~Mesh();
    Mesh(const Mesh&) = default;
    Mesh(Mesh&&) = default;
    Mesh& operator = (const Mesh&) = default;
    Mesh& operator = (Mesh&&) = default;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;

    GLuint texture = 0;
    GLenum primitiveType = GL_TRIANGLES;

    std::vector<vertex_t> vertices;
    std::vector<std::uint16_t> indices;

    glm::mat4 modelMatrix = glm::mat4(1.f);
};

struct RoomData final
{
    enum Flags
    {
        North = 0x1,
        East = 0x2,
        South = 0x4,
        West = 0x8,
        Ceiling = 0x10
    };

    std::int32_t flags = 0;
    std::array<float, 3> skyColour = {1.f, 1.f, 1.f};
    std::array<float, 3> roomColour = { 1.f, 1.f, 1.f };
    std::int32_t id = -1;
};