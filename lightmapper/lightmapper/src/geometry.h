#pragma once

#include "structures.h"
#include "scene.h"

#include <stdint.h>

#include <vector>

void updateGeometry(int32_t flags, Scene& scene);

void addNorthWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addEastWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addSouthWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addWestWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addCeiling(std::vector<Vertex>&, std::vector<std::uint16_t>&);

void addLight(Scene&);