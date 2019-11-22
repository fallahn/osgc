#include "geometry.h"
#include "GLCheck.hpp"
#include "structures.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


#include <iostream>
#include <functional>

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

    auto& verts = light->vertices;
    auto& indices = light->indices;
    light->primitiveType = GL_TRIANGLE_STRIP;

    const float radius = 0.1f;
    const std::size_t resolution = 5;
    static const float degToRad = M_PI / 180.f;

    std::size_t offset = 0; //offset into index array as we add faces
    auto buildFace = [&](const glm::mat4& rotation, const glm::vec2& uvOffset)
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        const float spacing = 1.f / resolution;
        const glm::vec2 uvSpacing((1.f / 2.f) / static_cast<float>(resolution), (1.f / 3.f) / static_cast<float>(resolution));
        for (auto y = 0u; y <= resolution; ++y)
        {
            for (auto x = 0u; x <= resolution; ++x)
            {
                positions.emplace_back((x * spacing) - 0.5f, (y * spacing) - 0.5f, 0.5f);
                uvs.emplace_back(uvOffset.x + (uvSpacing.x * x), uvOffset.y + (uvSpacing.y * y));
            }
        }

        for (auto i = 0u; i < positions.size(); ++i)
        {
            auto vertPos = rotation * glm::vec4(glm::normalize(positions[i]), 1.f);
            auto& vert = verts.emplace_back();
            vert.position[0] = vertPos.x * radius;
            vert.position[1] = vertPos.y * radius;
            vert.position[2] = vertPos.z * radius;
            vert.position[1] += 0.4f; //moe towards ceiling
            
            ////normals
            //vertexData.emplace_back(vert.x);
            //vertexData.emplace_back(vert.y);
            //vertexData.emplace_back(vert.z);

            ////because we're on a grid the tan points to
            ////the next vertex position - unless we're
            ////at the end of a line
            //glm::vec3 tan;
            //if (i % (resolution + 1) == (resolution))
            //{
            //    //end of the line
            //    glm::vec3 a = glm::normalize(positions[i - 1]);
            //    glm::vec3 b = glm::normalize(positions[i]);

            //    tan = glm::normalize(a - b) * rotation;
            //    tan = glm::reflect(tan, vert);
            //}
            //else
            //{
            //    glm::vec3 a = glm::normalize(positions[i + 1]);
            //    glm::vec3 b = glm::normalize(positions[i]);
            //    tan = glm::normalize(a - b) * rotation;
            //}
            //glm::vec3 bitan = glm::cross(tan, vert);

            //vertexData.emplace_back(tan.x);
            //vertexData.emplace_back(tan.y);
            //vertexData.emplace_back(tan.z);

            //vertexData.emplace_back(bitan.x);
            //vertexData.emplace_back(bitan.y);
            //vertexData.emplace_back(bitan.z);

            //UVs
            vert.texCoord[0] = uvs[i].x;
            vert.texCoord[1] = uvs[i].y;
        }

        //update indices
        for (auto y = 0u; y <= resolution; ++y)
        {
            auto start = y * resolution;
            for (auto x = 0u; x <= resolution; ++x)
            {
                indices.push_back(static_cast<std::uint16_t>(offset + start + resolution + x));
                indices.push_back(static_cast<std::uint16_t>(offset + start + x));
            }

            //add a degenerate at the end of the row bar last
            if (y < resolution - 1)
            {
                indices.push_back(static_cast<std::uint16_t>(offset + ((y + 1) * resolution + (resolution - 1))));
                indices.push_back(static_cast<std::uint16_t>(offset + ((y + 1) * resolution)));
            }
        }
        offset += positions.size();
    };

    //build each face rotating as we go
    //mapping to 2x3 texture atlas
    const float u = 1.f / 2.f;
    const float v = 1.f / 3.f;
    buildFace(glm::mat4(1.f), { 0.f, v * 2.f }); //Z+

    glm::mat4 rotation = glm::rotate(glm::mat4(1.f), degToRad * 90.f, glm::vec3(0.f, 1.f, 0.f));
    buildFace(rotation, { u, 0.f }); //X-

    rotation = glm::rotate(glm::mat4(1.f), degToRad * 180.f, glm::vec3(0.f, 1.f, 0.f));
    buildFace(rotation, { u, v * 2.f }); //Z-

    rotation = glm::rotate(glm::mat4(1.f), degToRad * 270.f, glm::vec3(0.f, 1.f, 0.f));
    buildFace(rotation, {}); //X+

    rotation = glm::rotate(glm::mat4(1.f), degToRad * 90.f, glm::vec3(1.f, 0.f, 0.f));
    buildFace(rotation, { u, v }); //Y-

    rotation = glm::rotate(glm::mat4(1.f), degToRad * -90.f, glm::vec3(1.f, 0.f, 0.f));
    buildFace(rotation, { 0.f, v }); //Y+


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