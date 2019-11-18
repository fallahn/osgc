#pragma once

#include "structures.h"
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>

int initScene(scene_t& scene);
void drawScene(const scene_t& scene, const glm::mat4&, const glm::mat4&);
void destroyScene(scene_t& scene);

int bake(scene_t& scene);