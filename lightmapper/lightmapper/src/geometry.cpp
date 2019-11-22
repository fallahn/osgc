#include "geometry.h"
#include "GLCheck.hpp"
#include "structures.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace
{
    static const float DefaultRoomWidth = 960.f;
    static const float DefaultRoomHeight = 540.f;
    static const float DefaultWallThickness = 12.f;

    static const float RoomWidth = 1.f;
    static const float RoomHeight = DefaultRoomHeight / DefaultRoomWidth;
    static const float WallThickness = DefaultWallThickness / DefaultRoomWidth;

    static const float DefaultTexWidth = 3000.f;
    static const float DefaultTexHeight = 2040.f;
}

void updateGeometry(const RoomData& room, Scene& scene)
{
    auto& meshes = scene.getMeshes();
    meshes.clear();

    auto& mesh = meshes.emplace_back(std::make_unique<Mesh>());

    auto& verts = mesh->vertices;
    auto& indices = mesh->indices;


    //create floor
    vertex_t vert;
    vert.position[0] = RoomWidth / 2.f;
    vert.position[2] = RoomWidth / 2.f;
    
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[2] -= RoomWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[0] -= RoomWidth;
    vert.texCoord[0] = DefaultRoomHeight / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[2] += RoomWidth;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(2);
    indices.push_back(3);
    indices.push_back(0);

    //create walls / ceiling based on flags
    auto flags = room.flags;
    if (flags & RoomData::Flags::North)
    {
        addNorthWall(verts, indices);
    }
    if (flags & RoomData::Flags::East)
    {
        addEastWall(verts, indices);
    }
    if (flags & RoomData::Flags::South)
    {
        addSouthWall(verts, indices);
    }
    if (flags & RoomData::Flags::West)
    {
        addWestWall(verts, indices);
    }
    if (flags & RoomData::Flags::Ceiling)
    {
        addCeiling(verts, indices);
    }

    //update buffers
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vertex_t), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    //do this afterwards just so we don't mess with room mesh mid-creation
    if (flags & RoomData::Flags::Ceiling)
    {
        addLight(room, scene);
    }
}

void addNorthWall(std::vector<Vertex>& verts, std::vector<std::uint16_t>& indices)
{
    //west edge
    std::uint16_t firstIndex = static_cast<std::uint16_t>(verts.size());
    
    Vertex vert;
    vert.position[0] = -RoomWidth / 2.f;
    vert.position[2] = vert.position[0] + WallThickness;

    vert.texCoord[0] = DefaultRoomHeight / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[2] -= WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight - DefaultWallThickness) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //main wall
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = RoomWidth / 2.f;
    vert.position[1] = 0.f;
    vert.position[2] = -vert.position[0] + WallThickness;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[0] = -RoomWidth / 2.f;
    vert.texCoord[0] = DefaultRoomHeight / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //east edge
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = RoomWidth / 2.f;
    vert.position[1] = 0.f;
    vert.position[2] = -RoomWidth / 2.f;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth + DefaultWallThickness) / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[2] += WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);
}

void addEastWall(std::vector<Vertex>& verts, std::vector<std::uint16_t>& indices)
{
    //north edge
    std::uint16_t firstIndex = static_cast<std::uint16_t>(verts.size());

    Vertex vert;
    vert.position[0] = (RoomWidth / 2.f) - WallThickness;
    vert.position[2] = -RoomWidth / 2.f;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[0] += WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth + DefaultWallThickness) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //main wall
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = (RoomWidth / 2.f) - WallThickness;
    vert.position[1] = 0.f;
    vert.position[2] = RoomWidth / 2.f;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[2] = -RoomWidth / 2.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //south edge
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = RoomWidth / 2.f;
    vert.position[1] = 0.f;
    vert.position[2] = RoomWidth / 2.f;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth + DefaultWallThickness) / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[0] -= WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);
}

void addSouthWall(std::vector<Vertex>& verts, std::vector<std::uint16_t>& indices)
{
    //east edge
    std::uint16_t firstIndex = static_cast<std::uint16_t>(verts.size());

    Vertex vert;
    vert.position[0] = RoomWidth / 2.f;
    vert.position[2] = vert.position[0] - WallThickness;

    vert.texCoord[0] = DefaultRoomHeight / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[2] += WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight - DefaultWallThickness) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //main wall
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = -RoomWidth / 2.f;
    vert.position[1] = 0.f;
    vert.position[2] = -vert.position[0] - WallThickness;
    vert.texCoord[0] = DefaultRoomHeight / DefaultTexWidth;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;

    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 1.f;
    vert.texCoord[1] -= 0.0001f; //hmm having a UV exactly on 1 causes an odd bug?

    verts.push_back(vert);

    vert.position[0] = RoomWidth / 2.f;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //west edge
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = -RoomWidth / 2.f;
    vert.position[1] = 0.f;
    vert.position[2] = RoomWidth / 2.f;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth + DefaultWallThickness) / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[2] -= WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);
}

void addWestWall(std::vector<Vertex>& verts, std::vector<std::uint16_t>& indices)
{
    //south edge
    std::uint16_t firstIndex = static_cast<std::uint16_t>(verts.size());

    Vertex vert;
    vert.position[0] = (-RoomWidth / 2.f) + WallThickness;
    vert.position[2] = RoomWidth / 2.f;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth + DefaultWallThickness) / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[0] -= WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //main wall
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = (-RoomWidth / 2.f) + WallThickness;
    vert.position[1] = 0.f;
    vert.position[2] = -RoomWidth / 2.f;

    vert.texCoord[0] = DefaultRoomHeight / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[0] = 0.f;
    verts.push_back(vert);

    vert.position[2] = RoomWidth / 2.f;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[0] = DefaultRoomHeight / DefaultTexWidth;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);

    //north edge
    firstIndex = static_cast<std::uint16_t>(verts.size());

    vert.position[0] = -RoomWidth / 2.f;
    vert.position[1] = 0.f;
    vert.position[2] = -RoomWidth / 2.f;

    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth + DefaultWallThickness) / DefaultTexWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[1] = RoomHeight;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[0] += WallThickness;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[1] = 0.f;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);
}

void addCeiling(std::vector<Vertex>& verts, std::vector<std::uint16_t>& indices)
{
    auto firstIndex = static_cast<std::uint16_t>(verts.size());

    Vertex vert;
    vert.position[0] = -RoomWidth / 2.f;
    vert.position[1] = RoomHeight;
    vert.position[2] = RoomWidth / 2.f;

    vert.texCoord[0] = 1.f - 0.0001f;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[2] -= RoomWidth;
    vert.texCoord[1] = DefaultRoomHeight / DefaultTexHeight;
    verts.push_back(vert);

    vert.position[0] += RoomWidth;
    vert.texCoord[0] = (DefaultRoomHeight + DefaultRoomHeight + DefaultRoomWidth) / DefaultTexWidth;
    verts.push_back(vert);

    vert.position[2] += RoomWidth;
    vert.texCoord[1] = (DefaultRoomHeight + DefaultRoomWidth) / DefaultTexHeight;
    verts.push_back(vert);

    indices.push_back(firstIndex + 0);
    indices.push_back(firstIndex + 1);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 2);
    indices.push_back(firstIndex + 3);
    indices.push_back(firstIndex + 0);
}

void addLight(const RoomData& room, Scene& scene)
{
    //add interior light
    auto& light = scene.getMeshes().emplace_back(std::make_unique<Mesh>());
    //TODO make a more complex piece of geom
    auto& verts = light->vertices;
    auto& indices = light->indices;

    const float lightWidth = 0.3f;

    Vertex vert;
    vert.position[0] = -lightWidth / 2.f;
    vert.position[1] = RoomHeight - 0.01f;
    vert.position[2] = lightWidth / 2.f;

    vert.texCoord[0] = 1.f;
    vert.texCoord[1] = 1.f;
    verts.push_back(vert);

    vert.position[2] -= lightWidth;
    vert.texCoord[1] = 0.f;
    verts.push_back(vert);

    vert.position[0] += lightWidth;
    vert.texCoord[0] = 0.f;
    verts.push_back(vert);

    vert.position[2] += lightWidth;
    vert.texCoord[1] = 1.f;
    verts.push_back(vert);

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(2);
    indices.push_back(3);
    indices.push_back(0);

    glBindBuffer(GL_ARRAY_BUFFER, light->vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vertex_t), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, light->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //update texture with light colour
    glBindTexture(GL_TEXTURE_2D, light->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, room.roomColour.data());
}