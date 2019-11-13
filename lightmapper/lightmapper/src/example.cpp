#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "example.h"
#include "gl_helpers.h"
#include "geometry.h"

#define LIGHTMAPPER_IMPLEMENTATION
#define LM_DEBUG_INTERPOLATION
#include "lightmapper.h"

int bake(scene_t *scene)
{
	lm_context *ctx = lmCreate(
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

	int w = scene->w, h = scene->h;
	float *data = (float*)calloc(w * h * 4, sizeof(float));
	lmSetTargetLightmap(ctx, data, w, h, 4);

	lmSetGeometry(ctx, NULL,                                                                 // no transformation in this example
		LM_FLOAT, (unsigned char*)scene->vertices.data() + offsetof(vertex_t, position), sizeof(vertex_t),
		LM_NONE , NULL                                                   , 0               , // no interpolated normals in this example
		LM_FLOAT, (unsigned char*)scene->vertices.data() + offsetof(vertex_t, texCoord), sizeof(vertex_t),
		scene->indices.size(), LM_UNSIGNED_SHORT, scene->indices.data());

	int vp[4];
	float view[16], projection[16];
	double lastUpdateTime = 0.0;
	while (lmBegin(ctx, vp, view, projection))
	{
		// render to lightmapper framebuffer
		glViewport(vp[0], vp[1], vp[2], vp[3]);
		drawScene(scene, view, projection);

		// display progress every second (printf is expensive)
		double time = glfwGetTime();
		if (time - lastUpdateTime > 1.0)
		{
			lastUpdateTime = time;
			printf("\r%6.2f%%", lmProgress(ctx) * 100.0f);
			fflush(stdout);
		}

		lmEnd(ctx);
	}
	printf("\rFinished baking %d triangles.\n", scene->indices.size() / 3);

	lmDestroy(ctx);

	// postprocess texture
	float *temp = (float*)calloc(w * h * 4, sizeof(float));
	for (int i = 0; i < 16; i++)
	{
		lmImageDilate(data, temp, w, h, 4);
		lmImageDilate(temp, data, w, h, 4);
	}
	lmImageSmooth(data, temp, w, h, 4);
	lmImageDilate(temp, data, w, h, 4);
	lmImagePower(data, w, h, 4, 1.0f / 2.2f, 0x7); // gamma correct color channels
	free(temp);

	// save result to a file
    if (lmImageSaveTGAf("result.tga", data, w, h, 4, 1.0f))
    {
        printf("Saved result.tga\n");
    }

	// upload result
	glBindTexture(GL_TEXTURE_2D, scene->lightmap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, data);
	free(data);

	return 1;
}

void error_callback(int error, const char *description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void mainLoop(GLFWwindow *window, scene_t *scene)
{
	glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        //TODO loop through all flag values
        //and pass current value for correct file name
        //TODO rebuild geom for each value
        bake(scene);
    }

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);

	// camera for glfw window
	float view[16], projection[16];
	fpsCameraViewMatrix(window, view);
	perspectiveMatrix(projection, 45.0f, (float)w / (float)h, 0.01f, 100.0f);

	// draw to screen with a blueish sky
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawScene(scene, view, projection);

	glfwSwapBuffers(window);
}

int main(int argc, char* argv[])
{
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
	{
		fprintf(stderr, "Could not initialize GLFW.\n");
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 8);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);
	glfwWindowHint(GLFW_STENCIL_BITS, GLFW_DONT_CARE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow *window = glfwCreateWindow(800, 600, "Lightmapping Example", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "Could not create window.\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);

	scene_t scene = {0};
	if (!initScene(&scene))
	{
		fprintf(stderr, "Could not initialize scene.\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	printf("Ambient Occlusion Baking Example.\n");
	printf("Use your mouse and the W, A, S, D, E, Q keys to navigate.\n");
	printf("Press SPACE to start baking one light bounce!\n");
	printf("This will take a few seconds and bake a lightmap illuminated by:\n");
	printf("1. The mesh itself (initially black)\n");
	printf("2. A white sky (1.0f, 1.0f, 1.0f)\n");

	while (!glfwWindowShouldClose(window))
	{
		mainLoop(window, &scene);
	}

	destroyScene(&scene);
	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}


int initScene(scene_t *scene)
{
	// load mesh
	/*if (!loadSimpleObjFile("assets/gazebo.obj", &scene->vertices, &scene->vertexCount, &scene->indices, &scene->indexCount))
	{
		fprintf(stderr, "Error loading obj file\n");
		return 0;
	}*/

	glGenVertexArrays(1, &scene->vao);
	glBindVertexArray(scene->vao);

	glGenBuffers(1, &scene->vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, scene->vbo);
	//glBufferData(GL_ARRAY_BUFFER, scene->vertexCount * sizeof(vertex_t), scene->vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &scene->ibo);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->ibo);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, scene->indexCount * sizeof(unsigned short), scene->indices, GL_STATIC_DRAW);

    //temp to test mesh generation
    updateGeometry(3, scene);

    //position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, position));
	//UV
    glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)offsetof(vertex_t, texCoord));

	//create lightmap texture - TODO set correct size
	scene->w = 3000;
	scene->h = 2040;
	glGenTextures(1, &scene->lightmap);
	glBindTexture(GL_TEXTURE_2D, scene->lightmap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	unsigned char emissive[] = { 0, 0, 0, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, emissive);

	// load shader
	const char *vp =
		"#version 150 core\n"
		"in vec3 a_position;\n"
		"in vec2 a_texcoord;\n"
		"uniform mat4 u_view;\n"
		"uniform mat4 u_projection;\n"
		"out vec2 v_texcoord;\n"

		"void main()\n"
		"{\n"
		"gl_Position = u_projection * (u_view * vec4(a_position, 1.0));\n"
		"v_texcoord = a_texcoord;\n"
		"}\n";

	const char *fp =
		"#version 150 core\n"
		"in vec2 v_texcoord;\n"
		"uniform sampler2D u_lightmap;\n"
		"out vec4 o_color;\n"

		"void main()\n"
		"{\n"
		"o_color = vec4(texture(u_lightmap, v_texcoord).rgb, gl_FrontFacing ? 1.0 : 0.0);\n"
		"}\n";

	const char *attribs[] =
	{
		"a_position",
		"a_texcoord"
	};

	scene->program = loadProgram(vp, fp, attribs, 2);
	if (!scene->program)
	{
		fprintf(stderr, "Error loading shader\n");
		return 0;
	}
	scene->u_view = glGetUniformLocation(scene->program, "u_view");
	scene->u_projection = glGetUniformLocation(scene->program, "u_projection");
	scene->u_lightmap = glGetUniformLocation(scene->program, "u_lightmap");

	return 1;
}

void drawScene(scene_t *scene, float *view, float *projection)
{
	glEnable(GL_DEPTH_TEST);

	glUseProgram(scene->program);
	glUniform1i(scene->u_lightmap, 0);
	glUniformMatrix4fv(scene->u_projection, 1, GL_FALSE, projection);
	glUniformMatrix4fv(scene->u_view, 1, GL_FALSE, view);

	glBindTexture(GL_TEXTURE_2D, scene->lightmap);

	glBindVertexArray(scene->vao);
	glDrawElements(GL_TRIANGLES, scene->indices.size(), GL_UNSIGNED_SHORT, 0);
}

void destroyScene(scene_t *scene)
{
	glDeleteVertexArrays(1, &scene->vao);
	glDeleteBuffers(1, &scene->vbo);
	glDeleteBuffers(1, &scene->ibo);
	glDeleteTextures(1, &scene->lightmap);
	glDeleteProgram(scene->program);
}