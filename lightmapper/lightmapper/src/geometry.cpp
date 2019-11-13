#include "geometry.h"

#include <vector>

namespace
{
    static const float RoomWidth = 1.f;
}

void updateGeometry(int32_t flags, scene_t* scene)
{
    //clear the scene arrays
    if (scene->vertices)
    {
        free(scene->vertices);
    }
    if (scene->indices)
    {
        free(scene->indices);
    }

    scene->vertexCount = 0;
    scene->indexCount = 0;

    //create floor
    std::vector<vertex_t> vertices;
    vertex_t vert;
    vert.p[0] = RoomWidth / 2.f;
    vert.p[1] = RoomWidth / 2.f;
    vert.p[2] = 0.f;
    vert.t[0] = 1.f;
    vert.t[1] = 0.f;
    vertices.push_back(vert);

    vert.p[1] -= RoomWidth;
    vert.t[1] = 1.f;
    vertices.push_back(vert);

    vert.p[0] -= RoomWidth;
    vert.p[1] += RoomWidth;
    vert.t[0] = 0.f;
    vertices.push_back(vert);

    vert.p[1] -= RoomWidth;
    vert.t[1] = 0.f;

    //TODO create walls / ceiling based on flags

    scene->vertexCount = 4;
    scene->vertices = (vertex_t*)calloc(4, sizeof(vertex_t));
    std::memcpy((void*)scene->vertices, vertices.data(), 4 * sizeof(vertex_t));

    scene->indexCount = 4;
    scene->indices = (unsigned short*)calloc(6, sizeof(unsigned short));
    scene->indices[0] = 0;
    scene->indices[1] = 1;
    scene->indices[2] = 2;
    scene->indices[3] = 1;
    scene->indices[4] = 3;
    scene->indices[5] = 2;

    //update buffers
    glBindBuffer(GL_ARRAY_BUFFER, scene->vbo);
    glBufferData(GL_ARRAY_BUFFER, scene->vertexCount * sizeof(vertex_t), scene->vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, scene->indexCount * sizeof(unsigned short), scene->indices, GL_STATIC_DRAW);
}