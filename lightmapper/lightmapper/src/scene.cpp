#include "structures.h"
#include "gl_helpers.h"
#include "geometry.h"
#include "lightmapper.h"
#include "GLCheck.hpp"

#include <assert.h>

bool Scene::init()
{
    //temp to test mesh generation
    updateGeometry(19, *this);

    m_lightmapWidth = 750;
    m_lightmapHeight = 510;

    //load shader
    const char* vp =
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

    const char* fp =
        "#version 150 core\n"
        "in vec2 v_texcoord;\n"
        "uniform sampler2D u_texture;\n"
        "out vec4 o_color;\n"

        "void main()\n"
        "{\n"
        "o_color = vec4(texture(u_texture, v_texcoord).rgb, gl_FrontFacing ? 1.0 : 0.0);\n"
        "}\n";

    const char* attribs[] =
    {
        "a_position",
        "a_texcoord"
    };

    m_programID = loadProgram(vp, fp, attribs, 2);
    if (!m_programID)
    {
        std::cout << "Error loading shader\n";
        return false;
    }
    m_modelUniform = glGetUniformLocation(m_programID, "u_model");
    m_viewUniform = glGetUniformLocation(m_programID, "u_view");
    m_projectionUniform = glGetUniformLocation(m_programID, "u_projection");
    m_textureUniform = glGetUniformLocation(m_programID, "u_texture");

    return true;
}

void Scene::draw(const glm::mat4& view, const glm::mat4& projection) const
{
    glEnable(GL_DEPTH_TEST);

    glUseProgram(m_programID);
    glUniform1i(m_textureUniform, 0);

    glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(m_viewUniform, 1, GL_FALSE, &view[0][0]);

    //foreach mesh in scene
    for (const auto& mesh : m_meshes)
    {
        //model matrix
        glUniformMatrix4fv(m_modelUniform, 1, GL_FALSE, &mesh->modelMatrix[0][0]);

        glBindTexture(GL_TEXTURE_2D, mesh->texture);

        glCheck(glBindVertexArray(mesh->vao));
        glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_SHORT, 0);
    }
}

void Scene::destroy()
{
    m_meshes.clear();

    glDeleteProgram(m_programID);
}

bool Scene::bake() const
{
    //reset the geometry texture to black else previous
    //bakes contribute to the emissions
    glBindTexture(GL_TEXTURE_2D, m_meshes[0]->texture);
    unsigned char emissive[] = { 0, 0, 0, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emissive);
    glBindTexture(GL_TEXTURE_2D, 0);

    lm_context* ctx = lmCreate(
        64,               // hemisphere resolution (power of two, max=512)
        0.001f, 100.0f,   // zNear, zFar of hemisphere cameras
        1.0f, 1.0f, 1.0f, // background color (white for ambient occlusion)
        2, 0.01f,         // lightmap interpolation threshold (small differences are interpolated rather than sampled)
                          // check debug_interpolation.tga for an overview of sampled (red) vs interpolated (green) pixels.
        0.0f);            // modifier for camera-to-surface distance for hemisphere rendering.
                          // tweak this to trade-off between interpolated normals quality and other artifacts (see declaration).

    if (!ctx)
    {
        std::cout << "Error: Could not initialize lightmapper.\n";
        return 0;
    }

    int w = m_lightmapWidth, h = m_lightmapHeight;

    std::vector<float> data(w * h * 4);
    lmSetTargetLightmap(ctx, data.data(), w, h, 4);

    //assume the room mesh is always in the first slot...
    assert(!m_meshes.empty());
    const auto& mesh = m_meshes[0];
    lmSetGeometry(ctx, NULL,                                                                 // no transformation in this example
        LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, position), sizeof(vertex_t),
        LM_NONE, NULL, 0, // no interpolated normals in this example
        LM_FLOAT, (unsigned char*)mesh->vertices.data() + offsetof(vertex_t, texCoord), sizeof(vertex_t),
        mesh->indices.size(), LM_UNSIGNED_SHORT, mesh->indices.data());

    int vp[4];
    glm::mat4 view, projection;
    double lastUpdateTime = 0.0;
    while (lmBegin(ctx, vp, &view[0][0], &projection[0][0]))
    {
        //render to lightmapper framebuffer
        glViewport(vp[0], vp[1], vp[2], vp[3]);
        draw(view, projection);

        //display progress every second (printf is expensive)
        double time = glfwGetTime();
        if (time - lastUpdateTime > 1.0)
        {
            lastUpdateTime = time;
            printf("\r%6.2f%%", lmProgress(ctx) * 100.0f);
            fflush(stdout);
        }

        lmEnd(ctx);
    }
    printf("\rFinished baking %zd triangles.\n", mesh->indices.size() / 3);

    lmDestroy(ctx);

    // postprocess texture
    std::vector<float> temp(data.size());
    for (int i = 0; i < 16; i++)
    {
        lmImageDilate(data.data(), temp.data(), w, h, 4);
        lmImageDilate(temp.data(), data.data(), w, h, 4);
    }
    lmImageSmooth(data.data(), temp.data(), w, h, 4);
    lmImageDilate(temp.data(), data.data(), w, h, 4);
    lmImagePower(data.data(), w, h, 4, 1.0f / 2.2f, 0x7); // gamma correct color channels


    // save result to a file
    if (lmImageSaveTGAf("result.tga", data.data(), w, h, 4, 1.0f))
    {
        std::cout << "Saved result.tga\n";
    }

    //upload result to preview texture
    glBindTexture(GL_TEXTURE_2D, mesh->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, data.data());

    return 1;
}