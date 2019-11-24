#pragma once

#include "structures.h"
#include "scene.h"

#include "glm/vec3.hpp"

#include <stdint.h>

#include <vector>

void buildRoom(const RoomData&, Scene&, glm::vec3 offset = glm::vec3(0.f));
void addNorthWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addEastWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addSouthWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addWestWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addCeiling(std::vector<Vertex>&, std::vector<std::uint16_t>&);

void addLight(const RoomData&, Scene&, glm::vec3);
void addModel(const ModelData&, Scene&, glm::vec3);