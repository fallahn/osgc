#include "scene.h"
#include "gl_helpers.h"
#include "geometry.h"
#include "lightmapper.h"
#include "GLCheck.hpp"

#include <assert.h>

int initScene(scene_t& scene)
{
    //temp to test mesh generation
    updateGeometry(19, scene);

    scene.lightmapWidth = 750;
    scene.lightmapHeight = 510;

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

    scene.program = loadProgram(vp, fp, attribs, 2);
    if (!scene.program)
    {
        fprintf(stderr, "Error loading shader\n");
        return 0;
    }
    scene.u_model = glGetUniformLocation(scene.program, "u_model");
    scene.u_view = glGetUniformLocation(scene.program, "u_view");
    scene.u_projection = glGetUniformLocation(scene.program, "u_projection");
    scene.u_texture = glGetUniformLocation(scene.program, "u_texture");

    return 1;
}

void drawScene(const scene_t& scene, const glm::mat4& view, const glm::mat4& projection)
{
    glEnable(GL_DEPTH_TEST);

    glUseProgram(scene.program);
    glUniform1i(scene.u_texture, 0);

    glUniformMatrix4fv(scene.u_projection, 1, GL_FALSE, &projection[0][0]);
    glUniformMatrix4fv(scene.u_view, 1, GL_FALSE, &view[0][0]);

    //foreach mesh in scene
    for (const auto& mesh : scene.meshes)
    {
        //model matrix
        glUniformMatrix4fv(scene.u_model, 1, GL_FALSE, &mesh->modelMatrix[0][0]);

        glBindTexture(GL_TEXTURE_2D, mesh->texture);

        glCheck(glBindVertexArray(mesh->vao));
        glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_SHORT, 0);
    }
}

void destroyScene(scene_t& scene)
{
    scene.meshes.clear();

    glDeleteProgram(scene.program);
}

int bake(scene_t& scene)
{
    //reset the geometry texture to black else previous
    //bakes contribute to the emissions
    glBindTexture(GL_TEXTURE_2D, scene.meshes[0]->texture);
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
        fprintf(stderr, "Error: Could not initialize lightmapper.\n");
        return 0;
    }

    int w = scene.lightmapWidth, h = scene.lightmapHeight;

    std::vector<float> data(w * h * 4);
    lmSetTargetLightmap(ctx, data.data(), w, h, 4);

    //assume the room mesh is always in the first slot...
    assert(!scene.meshes.empty());
    const auto& mesh = scene.meshes[0];
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
        drawScene(scene, view, projection);

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
        printf("Saved result.tga\n");
    }

    //upload result to preview texture
    glBindTexture(GL_TEXTURE_2D, mesh->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, data.data());

    return 1;
}