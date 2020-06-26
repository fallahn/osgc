#pragma once

#include "structures.h"
#include "scene.h"

#include "glm/vec3.hpp"

#include <stdint.h>

#include <vector>

//scale the 1024 size texture as if the room was 960 units
//kludgy I know but it'll do... this is why blender models
//have to be exported at a 0.667 scale...
static constexpr float RoomGeometrySize = (1024.f / 960.f) * 10.f;
static constexpr float OrthoSize = (RoomGeometrySize / 2.f) * 1.1f;

//used when loading map data from the 'doodlebob' gamein osgc
void buildRoom(const RoomData&, Scene&, glm::vec3 offset = glm::vec3(0.f));
void addNorthWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addEastWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addSouthWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addWestWall(std::vector<Vertex>&, std::vector<std::uint16_t>&);
void addCeiling(std::vector<Vertex>&, std::vector<std::uint16_t>&);

void addLight(const RoomData&, Scene&, glm::vec3);

//loads a model into the given scene. Only general purpose function
//not limited to the doodle bob game.
void addModel(const ModelData&, Scene&, glm::vec3);