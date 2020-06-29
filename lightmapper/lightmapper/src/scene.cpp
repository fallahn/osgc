#include "structures.h"
#include "geometry.h"
#include "lightmapper.h"
#include "GLCheck.hpp"
#include "stb_image_write.h"
#include "glm/gtc/matrix_transform.hpp"

#include <assert.h>

namespace
{
    glm::mat4 yUpMatrix = glm::mat4(1.f);
}

bool Scene::init()
{
    yUpMatrix = glm::rotate(glm::mat4(1.f), -90.f * ((float)M_PI / 180.f), glm::vec3(1.f, 0.f, 0.f));

    m_lightmapWidth = ConstVal::RoomTextureWidth;
    m_lightmapHeight = ConstVal::RoomTextureHeight;

    //load shader
    const char* vertexShader =
        "#version 150 core\n"
        "in vec3 a_position;\n"
        "in vec2 a_texcoord;\n"
        "uniform mat4 u_model;\n"
        "uniform mat4 u_view;\n"
        "uniform mat4 u_projection;\n"
        "out vec2 v_texcoord;\n"

        "void main()\n"
        "{\n"
        "gl_Position = u_projection * (u_view * u_model * vec4(a_position, 1.0));\n"
        "v_texcoord = a_texcoord;\n"
        "}\n";

    const char* fragShaderTextured =
        "#version 150 core\n"
        "in vec2 v_texcoord;\n"
        "uniform sampler2D u_texture;\n"
        "out vec4 o_color;\n"

        "void main()\n"
        "{\n"
        "vec4 colour = texture(u_texture, v_texcoord);"
        "if(colour.a < 0.5) discard;"
        "o_color = vec4(colour.rgb, gl_FrontFacing ? 1.0 : 0.0);\n"
        "}\n";

    const char* fragShaderColoured = 
        "#version 150 core\n"
        "uniform vec3 u_colour = vec3(1.0, 0.0, 0.0);\n"
        "out vec4 o_colour;\n"

        "void main()\n"
        "{\n"
        "o_colour = vec4(u_colour, 1.0);\n"
        "}\n";

    const char* attribs[] =
    {
        "a_position",
        "a_texcoord"
    };

    m_meshShader.load(vertexShader, fragShaderTextured, attribs, 2);
    if (!m_meshShader.programID)
    {
        std::cout << "Error loading mesh shader\n";
        return false;
    }

    m_rectShader.load(vertexShader, fragShaderColoured, attribs, 2);
    if (!m_rectShader.programID)
    {
        std::cout << "Failed compiling rectangle shader\n";
        return false;
    }

    return true;
}

void Scene::draw(const glm::mat4& view, const glm::mat4& projection, bool drawMeasure) const
{
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    glUseProgram(m_meshShader.programID);
    glUniform1i(m_meshShader.textureUniform, 0);

    glUniformMatrix4fv(m_meshShader.projectionUniform, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(m_meshShader.viewUniform, 1, GL_FALSE, &view[0][0]);

    //foreach mesh in scene
    for (const auto& mesh : m_meshes)
    {
        if (mesh->vao)
        {
            //model matrix
            glUniformMatrix4fv(m_meshShader.modelUniform, 1, GL_FALSE, &mesh->modelMatrix[0][0]);

            glBindTexture(GL_TEXTURE_2D, mesh->texture);

            glCheck(glBindVertexArray(mesh->vao));
            glDrawElements(mesh->primitiveType, mesh->indices.size(), GL_UNSIGNED_SHORT, 0);
        }
    }

    //draw measure mesh if it exists
    if (drawMeasure && m_measureMesh)
    {
        auto& mesh = m_measureMesh;
        mesh->modelMatrix = glm::mat4(1.f);
        if (!m_zUp)
        {
            mesh->modelMatrix *= yUpMatrix;
        }

        
        glUniformMatrix4fv(m_meshShader.modelUniform, 1, GL_FALSE, &mesh->modelMatrix[0][0]);

        glBindTexture(GL_TEXTURE_2D, mesh->texture);

        glCheck(glBindVertexArray(mesh->vao));
        glDrawElements(mesh->primitiveType, mesh->indices.size(), GL_UNSIGNED_SHORT, 0);
    }

    //glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    //render any rectangles
    glUseProgram(m_rectShader.programID);

    glUniformMatrix4fv(m_rectShader.projectionUniform, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(m_rectShader.viewUniform, 1, GL_FALSE, &view[0][0]);
    for (const auto& mesh : m_rectangles)
    {
        if (mesh->vao)
        {
            //model matrix
            auto modelMat = mesh->modelMatrix;
            if (!m_zUp)
            {
                modelMat *= yUpMatrix;
            }

            glUniformMatrix4fv(m_rectShader.modelUniform, 1, GL_FALSE, &modelMat[0][0]);

            //set colour per rectangle
            glUniform3f(m_rectShader.colourUniform, mesh->colour.r, mesh->colour.g, mesh->colour.b);

            glCheck(glBindVertexArray(mesh->vao));
            glCheck(glDrawElements(mesh->primitiveType, mesh->indices.size(), GL_UNSIGNED_SHORT, nullptr));
        }
    }
    glUseProgram(0);
}

void Scene::destroy()
{
    m_rectangles.clear();
    m_meshes.clear();
    m_measureMesh.reset();
}

bool Scene::bake(const std::string& output, const std::array<float, 3>& sky) const
{
    assert(!m_meshes.empty());

    //reset the geometry texture to black else previous
    //bakes contribute to the emissions
    glBindTexture(GL_TEXTURE_2D, m_meshes[0]->texture);
    unsigned char emissive[] = { 0, 0, 0, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emissive);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    lm_context* ctx = lmCreate(
        128,               // hemisphere resolution (power of two, max=512)
        0.001f, 100.0f,   // zNear, zFar of hemisphere cameras
        //1.0f, 1.0f, 1.0f, // background color (white for ambient occlusion)
        sky[0], sky[1], sky[2],
        2, 0.01f,         // lightmap interpolation threshold (small differences are interpolated rather than sampled)
                          // check debug_interpolation.tga for an overview of sampled (red) vs interpolated (green) pixels.
        0.0f);            // modifier for camera-to-surface distance for hemisphere rendering.
                          // tweak this to trade-off between interpolated normals quality and other artifacts (see declaration).

    if (!ctx)
    {
        std::cout << "Error: Could not initialize lightmapper.\n";
        return 0;
    }

    int w = m_lightmapWidth, h = m_lightmapHeight, c = 4;
    std::vector<float> data(w * h * c);

    //assume the target mesh is always in the first slot...
    assert(!m_meshes.empty());
    const auto& mesh = m_meshes[0];

    //for (auto b = 0; b < 4; ++b)
    {
        //for (const auto& mesh : m_meshes)
        {
            //std::memset(data.data(), 0, data.size() * sizeof(float));
            lmSetTargetLightmap(ctx, data.data(), w, h, c);

            if (mesh->hasNormals)
            {
                lmSetGeometry(ctx, &mesh->modelMatrix[0][0],
                    LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, position), sizeof(vertex_t),
                    //LM_NONE, NULL, 0, // no interpolated normals in this example
                    LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, normal), sizeof(vertex_t),
                    LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, texCoord), sizeof(vertex_t),
                    mesh->indices.size(), LM_UNSIGNED_SHORT, mesh->indices.data());
            }
            else
            {
                lmSetGeometry(ctx, &mesh->modelMatrix[0][0],
                    LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, position), sizeof(vertex_t),
                    LM_NONE, NULL, 0, // no interpolated normals in this example
                    //LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, normal), sizeof(vertex_t),
                    LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, texCoord), sizeof(vertex_t),
                    mesh->indices.size(), LM_UNSIGNED_SHORT, mesh->indices.data());
            }

            int vp[4];
            glm::mat4 view, projection;

            double lastUpdateTime = 0.0;
            while (lmBegin(ctx, vp, &view[0][0], &projection[0][0]))
            {
                //render to lightmapper framebuffer
                glViewport(vp[0], vp[1], vp[2], vp[3]);
                draw(view, projection, false);

                //display progress every second (printf is expensive)
                double time = glfwGetTime();
                if (time - lastUpdateTime > 1.0)
                {
                    lastUpdateTime = time;
                    printf("\r%6.2f%%", lmProgress(ctx) * 100.0f);
                    fflush(stdout);
                    //not that this is useful when running in the main thread...
                    m_progressString = std::to_string(lmProgress(ctx) * 100.f);
                }

                lmEnd(ctx);
            }

            // postprocess texture
            std::vector<float> temp(data.size());
            for (int i = 0; i < 16; i++)
            {
                lmImageDilate(data.data(), temp.data(), w, h, c);
                lmImageDilate(temp.data(), data.data(), w, h, c);
            }
            //upload result to preview texture - only really necessary in multi-pass bakes
            glBindTexture(GL_TEXTURE_2D, mesh->texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, data.data());

            printf("\rFinished baking %zd triangles.\n", mesh->indices.size() / 3);
        }
        //printf("\rFinished pass %zd of %zd.\n", b, 4);
    }
    m_progressString = "Finished";

    lmDestroy(ctx);

    std::vector<float> temp(data.size());
    lmImageSmooth(data.data(), temp.data(), w, h, c);
    lmImageDilate(temp.data(), data.data(), w, h, c);
    lmImagePower(data.data(), w, h, 4, 1.0f / 2.2f, 0x7); // gamma correct color channels

    //set all alpha values to 1
    for (auto i = 3u; i < data.size(); i += 4u)
    {
        data[i] = 1.f;
    }

    glBindTexture(GL_TEXTURE_2D, mesh->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, data.data());

    //save result to a file
    if (!output.empty())
    {
        auto fileName = output + ".png";
        std::vector<std::uint8_t> buffer(w * h * c);
        lmImageFtoUB(data.data(), buffer.data(), w, h, c, 1.f);

        stbi_write_png(fileName.c_str(), w, h, c, buffer.data(), w * c);
    }


    return 1;
}

void Scene::saveLightmap(const std::string& path)
{
    if (m_meshes.empty() || path.empty())
    {
        return;
    }

    std::vector<std::uint8_t> buffer(m_lightmapHeight * m_lightmapWidth * 4);
    glBindTexture(GL_TEXTURE_2D, m_meshes[0]->texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    stbi_write_png(path.c_str(), m_lightmapWidth, m_lightmapHeight, 4, buffer.data(), m_lightmapWidth * 4);
}

void Scene::createMeasureMesh(const std::string& path)
{
    if (!m_measureMesh)
    {
        m_measureMesh = std::make_unique<Mesh>();
        auto& mesh = m_measureMesh;

        auto& verts = mesh->vertices;
        auto& indices = mesh->indices;

        const float width = RoomGeometrySize; 

        //create floor
        vertex_t vert;
        vert.position[0] = width / 2.f;
        vert.position[2] = width / 2.f;

        vert.texCoord[0] = 1.f;
        vert.texCoord[1] = 2.f;
        verts.push_back(vert);

        vert.position[2] -= width;
        vert.texCoord[1] = 0.f;
        verts.push_back(vert);

        vert.position[0] -= width;
        vert.texCoord[0] = 0.f;
        verts.push_back(vert);

        vert.position[2] += width;
        vert.texCoord[1] = 2.f;
        verts.push_back(vert);

        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        indices.push_back(2);
        indices.push_back(3);
        indices.push_back(0);

        mesh->updateGeometry();
    }
    m_measureMesh->loadTexture(path);
}

void Scene::removeMeasureMesh()
{
    m_measureMesh.reset();
}

RectMesh* Scene::addRectangle()
{
    auto* rect = m_rectangles.emplace_back(std::make_unique<RectMesh>()).get();
    rect->updateVerts();
    return rect;
}

void Scene::removeRectangle(RectMesh* rect)
{
    if (rect != nullptr)
    {
        m_rectangles.erase(std::remove_if(m_rectangles.begin(), m_rectangles.end(), 
            [rect](const std::unique_ptr<RectMesh>& mesh)
            {
                return mesh.get() == rect;            
            }), m_rectangles.end());
    }
}