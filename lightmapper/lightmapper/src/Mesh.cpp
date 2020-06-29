#include "structures.h"
#include "GLCheck.hpp"
#include "stb_image.h"

Mesh::Mesh()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glCheck(glEnableVertexAttribArray(0));
    glCheck(glEnableVertexAttribArray(1));
    glCheck(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position)));
    glCheck(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, texCoord)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //TODO we could use some texture management to share the default texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    unsigned char emissive[] = { 0, 0, 0, 255 }; //defaults to emitting no light.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emissive);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Mesh::~Mesh()
{
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
}

void Mesh::updateGeometry()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindVertexArray(vao);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_t), vertices.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBindVertexArray(vao);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Mesh::loadTexture(const std::string& path)
{
    int w, h, n;
    if (auto* imgData = stbi_load(path.c_str(), &w, &h, &n, 0); imgData)
    {
        if (w > 0 && h > 0 && (n == 3 || n == 4))
        {
            //TODO we could assert the texture is the size we're expecting...
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            if (n == 3)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, imgData);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        stbi_image_free(imgData);
    }
}

void Mesh::setTextureSmooth(bool smooth)
{
    assert(texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    if (smooth)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RectMesh::updateVerts()
{
    const float handleOffset = RectMesh::handleSize;

    primitiveType = GL_LINES;

    vertices.clear();
    vertices.emplace_back().position = { start.x, 0.f, -start.y };
    vertices.emplace_back().position = { start.x, 0.f, -end.y };
    vertices.emplace_back().position = { end.x, 0.f, -end.y };
    vertices.emplace_back().position = { end.x, 0.f, -start.y };

    vertices.emplace_back().position = { start.x - handleOffset, 0.f, -start.y + handleOffset };
    vertices.emplace_back().position = { start.x - handleOffset, 0.f, -start.y };
    vertices.emplace_back().position = { start.x, 0.f, -start.y };
    vertices.emplace_back().position = { start.x, 0.f, -start.y + handleOffset };

    vertices.emplace_back().position = { end.x, 0.f, -end.y };
    vertices.emplace_back().position = { end.x, 0.f, -end.y - handleOffset };
    vertices.emplace_back().position = { end.x + handleOffset, 0.f, -end.y - handleOffset };
    vertices.emplace_back().position = { end.x + handleOffset, 0.f, -end.y };

    indices = 
    { 
        0,1,
        1,2, 
        2,3,
        3,0 ,

        4,5,
        5,6,
        6,7,
        7,4,

        8,9,
        9,10,
        10,11,
        11,8
    };

    updateGeometry();
}

bool RectMesh::contains(glm::vec2 point) const
{
    float minX = std::min(start.x, end.x);
    float maxX = std::max(start.x, end.x);
    float minY = std::min(start.y, end.y);
    float maxY = std::max(start.y, end.y);

    return (point.x >= minX) && (point.x < maxX) && (point.y >= minY) && (point.y < maxY);
}