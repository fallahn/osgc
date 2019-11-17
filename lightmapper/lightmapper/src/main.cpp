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

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>

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
    glm::mat4 view = fpsCameraViewMatrix(window);
    glm::mat4 projection = glm::perspective(45.f * static_cast<float>(M_PI/180.f), static_cast<float>(w) / static_cast<float>(h), 0.1f, 100.f);

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

    int w = 800;
    int h = 600;
    std::ifstream iFile("window", std::ios::binary);
    if (!iFile.fail())
    {
        iFile.read((char*)&w, sizeof(w));
        iFile.read((char*)&h, sizeof(h));
    }


	GLFWwindow *window = glfwCreateWindow(w, h, "Lightmapper", NULL, NULL);
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

    //save the current window size
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    std::ofstream oFile("window", std::ios::binary);
    if (oFile.is_open() && oFile.good())
    {
        std::vector<char>buff(sizeof(int)* 2);
        std::memcpy(buff.data(), &w, sizeof(int));
        std::memcpy(buff.data() + sizeof(int), &h, sizeof(int));
        oFile.write(buff.data(), buff.size());
    }

	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}