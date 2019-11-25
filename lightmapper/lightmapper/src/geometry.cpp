#include "geometry.h"
#include "App.hpp"
#include "GLCheck.hpp"
#include "structures.h"
#include "SerialVertex.hpp"
#include "String.hpp"
#include "OBJ_Loader.h"
namespace ol = objl;

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <functional>
#include <fstream>

namespace
{
    const float DefaultRoomWidth = 960.f;
    const float DefaultRoomHeight = 540.f;
    const float DefaultWallThickness = 12.f;

    const float RoomWidth = 10.f;
    const float RoomHeight = (DefaultRoomHeight / DefaultRoomWidth) * RoomWidth;
    const float WallThickness = (DefaultWallThickness / DefaultRoomWidth) * RoomWidth;

    const std::int32_t RoomsPerRow = 8;
    const std::int32_t RoomCount = RoomsPerRow * RoomsPerRow;
    const float Scale = RoomWidth / DefaultRoomWidth;

    const float DefaultTexWidth = 3000.f;
    const float DefaultTexHeight = 2040.f;
}

void App::updateSceneGeometry(const RoomData& room)
{
    auto& meshes = m_scene.getMeshes();
    meshes.clear();

    //build main room
    buildRoom(room, m_scene);

    //if walls are missing build peripheral rooms
    //to make sure they properly influence the light map
    for (auto i = 0u; i < 4; ++i)
    {
        if ((room.flags & (1 << i)) == 0)
        {
            auto index = -1;
            glm::vec3 position(0.f);
            switch (1 << i)
            {
            default: break;
            case RoomData::Flags::North:
                index = room.id - RoomsPerRow;
                position.z = -RoomWidth;
                break;
            case RoomData::Flags::East:
                //in *theory* this could be wrapping onto the next row
                //but in *theory* we shouldn't have a room with a missing
                //outside wall... so I'm going to skip this check :)
                index = room.id + 1;
                position.x = RoomWidth;
                break;
            case RoomData::Flags::South:
                index = room.id + RoomsPerRow;
                position.z = RoomWidth;
                break;
            case RoomData::Flags::West:
                index = room.id - 1;
                position.x = -RoomWidth;
                break;
            }

            if (index >= 0 && index < RoomCount)
            {
                buildRoom(m_mapData[index], m_scene, position);
                
            }
        }
    }
}

void App::importObjFile(const std::string& path)
{
    ol::Loader loader;
    if (loader.LoadFile(path)
        && !loader.LoadedMeshes.empty())
    {
        m_mapData.clear();
        m_scene.getMeshes().clear();
        m_currentRoom = -1;
        m_showImportWindow = false;

        m_importTransform = {};

        const auto& inVerts = loader.LoadedMeshes[0].Vertices;
        const auto& inIndices = loader.LoadedMeshes[0].Indices;

        if (!inVerts.empty() && !inIndices.empty())
        {
            auto& mesh = m_scene.getMeshes().emplace_back(std::make_unique<Mesh>());
            for (const auto& vert : inVerts)
            {
                auto& outVert = mesh->vertices.emplace_back();
                outVert.position[0] = vert.Position.X;
                outVert.position[1] = -vert.Position.Z;
                outVert.position[2] = vert.Position.Y;

                outVert.normal[0] = vert.Normal.X;
                outVert.normal[1] = -vert.Normal.Z;
                outVert.normal[2] = vert.Normal.Y;

                outVert.texCoord[0] = vert.TextureCoordinate.X;
                outVert.texCoord[1] = 1.f - vert.TextureCoordinate.Y;
            }

            for (auto i : inIndices)
            {
                mesh->indices.push_back(static_cast<std::uint16_t>(i));
            }
            //std::reverse(mesh->indices.begin(), mesh->indices.end());
            mesh->hasNormals = true;

            mesh->updateGeometry();

            //give it a different colour just to make pre-baked model visible
            glm::vec3 c(1.f, 1.f, 0.95f);
            glBindTexture(GL_TEXTURE_2D, mesh->texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, &c);

            m_showImportWindow = true;


            //create a ground plane... although we want to omit it from the bake?
            //maybe not, these objects are relative to the ground after all...
            auto& groundMesh = m_scene.getMeshes().emplace_back(std::make_unique<Mesh>());
            auto& verts = groundMesh->vertices;
            auto& indices = groundMesh->indices;

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

            groundMesh->updateGeometry();
        }
    }
}

void buildRoom(const RoomData& room, Scene& scene, glm::vec3 offset)
{
    auto& meshes = scene.getMeshes();
    auto& mesh = meshes.emplace_back(std::make_unique<Mesh>());
    mesh->modelMatrix = glm::translate(glm::mat4(1.f), offset);

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
    mesh->updateGeometry();


    //do this afterwards just so we don't mess with room mesh mid-creation
    if (flags & RoomData::Flags::Ceiling)
    {
        addLight(room, scene, offset);
    }

    //load any prop models
    for (const auto& model : room.models)
    {
        addModel(model, scene, offset);
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

void addLight(const RoomData& room, Scene& scene, glm::vec3 offsetPos)
{
    //add interior light
    auto& light = scene.getMeshes().emplace_back(std::make_unique<Mesh>());

    auto& verts = light->vertices;
   // std::vector<Vertex> verts;
    auto& indices = light->indices;
    light->primitiveType = GL_TRIANGLE_STRIP;
    light->modelMatrix = glm::translate(glm::mat4(1.f), offsetPos);

    const float radius = 1.f;
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
            vert.position[1] += RoomHeight * 0.8f; //move towards ceiling
            
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

    light->updateGeometry();

    //update texture with light colour
    glBindTexture(GL_TEXTURE_2D, light->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, room.roomColour.data());
}

void addModel(const ModelData& model, Scene& scene, glm::vec3 offset)
{
    //remember model z/y axis are switched. Because, y'know.
    std::ifstream file(model.path, std::ios::binary);
    if(file.is_open() && file.good())
    {
        auto fileSize = xy::Util::IO::getFileSize(file);
        if (fileSize > sizeof(std::size_t))
        {
            std::size_t vertCount = 0;
            file.read((char*)&vertCount, sizeof(vertCount));

            if (fileSize - sizeof(vertCount) == sizeof(SerialVertex) * vertCount)
            {
                std::vector<SerialVertex> buff(vertCount);
                file.read((char*)buff.data(), sizeof(SerialVertex) * vertCount);

                //unfortunately this format doesn't use indexed vertex data
                auto& mesh = scene.getMeshes().emplace_back(std::make_unique<Mesh>());
                auto& verts = mesh->vertices;
                auto& indices = mesh->indices;

                for (auto i = 0u; i < vertCount; ++i)
                {
                    indices.push_back(static_cast<std::uint16_t>(i));

                    auto& vert = verts.emplace_back();
                    vert.position[0] = buff[i].posX;
                    vert.position[2] = buff[i].posY;
                    vert.position[1] = model.depth * (static_cast<float>(buff[i].heightMultiplier) / 255.f);

                    //normal data
                    vert.normal[0] = static_cast<float>(buff[i].normX) / 255.f;
                    vert.normal[1] = static_cast<float>(buff[i].normZ) / 255.f;
                    vert.normal[2] = static_cast<float>(buff[i].normY) / 255.f;

                    vert.normal[0] *= 2.f;
                    vert.normal[0] -= 1.f;
                    vert.normal[1] *= 2.f;
                    vert.normal[1] -= 1.f;
                    vert.normal[2] *= 2.f;
                    vert.normal[2] -= 1.f;

                    vert.texCoord[0] = buff[i].texU;
                    vert.texCoord[1] = buff[i].texV;
                }
                mesh->hasNormals = true;

                mesh->updateGeometry();


                /*glm::vec3 c(0.2f, 0.2f, 0.8f);
                glBindTexture(GL_TEXTURE_2D, mesh->texture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, &c);*/

                //set the model position (and scale because the rooms are 1x1)
                mesh->modelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(model.position.x * Scale, 0.f, model.position.y * Scale));
                mesh->modelMatrix = glm::translate(mesh->modelMatrix, offset);
                mesh->modelMatrix = glm::rotate(mesh->modelMatrix, model.rotation * (static_cast<float>(M_PI) / 180.f), glm::vec3(0.f, 1.f, 0.f));
                mesh->modelMatrix = glm::scale(mesh->modelMatrix, glm::vec3(Scale, Scale, Scale));
            }
        }

        file.close();
    }
    else
    {
        std::cout << "failed to open " << model.path << "\n";
    }
}