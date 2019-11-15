#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "scene.h"
#include "gl_helpers.h"
#include "geometry.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <iostream>

#define LIGHTMAPPER_IMPLEMENTATION
#define LM_DEBUG_INTERPOLATION
#include "lightmapper.h"

void error_callback(int error, const char *description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void mainLoop(GLFWwindow* window, scene_t& scene)
{
	glfwPollEvents();
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        bake(scene);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    //TODO custom imgui rendering


    ImGui::Render();
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);

	//camera for glfw window
	float view[16], projection[16];
	fpsCameraViewMatrix(window, view);
	perspectiveMatrix(projection, 45.0f, (float)w / (float)h, 0.01f, 100.0f);

	//draw to screen with a blueish sky
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawScene(scene, view, projection);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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


	GLFWwindow *window = glfwCreateWindow(800, 600, "Lightmapper", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "Could not create window.\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
    //built in loader breaks ImGui
    //gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    gladLoadGL();

    const char* glslVer = "#version 150";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        std::cout << "failed glfw init\n";
    }
    if (!ImGui_ImplOpenGL3_Init(glslVer))
    {
        std::cout << "failed opengl init\n";
    }

	scene_t scene = {0};
	if (!initScene(scene))
	{
		fprintf(stderr, "Could not initialize scene.\n");
		glfwDestroyWindow(window);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	while (!glfwWindowShouldClose(window))
	{
		mainLoop(window, scene);
	}

	destroyScene(scene);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}