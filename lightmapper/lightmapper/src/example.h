#pragma once

#include "structures.h"
#include <GLFW/glfw3.h>

int initScene(scene_t* scene);
void drawScene(scene_t* scene, float* view, float* projection);
void destroyScene(scene_t* scene);