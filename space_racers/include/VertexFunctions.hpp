/*********************************************************************

Copyright 2019 Matt Marchant

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*********************************************************************/

#pragma once

//functions for creating '3D' sprites

/*
vertex positions are stored with the regular x/y components - the Z position is created
as a normlised value of 0 - 1 as a percentage of the max height of the mesh. This value
is stored in the vertex colour alpha channel. Vertex RGB channels are used to store normal
data about the vertices, so the model can be lit.
*/

#include <SFML/System/Vector3.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <vector>

//takes mesh model data and converts it to a vertex array in the 'Sprite3D' format.
std::vector<sf::Vertex> convertData(std::vector<sf::Vector3f>& positions, const std::vector<sf::Vector3f>& normals, const std::vector<sf::Vector2f>& UVs);

//creates the 'force field' around the starting line
std::vector<sf::Vertex> createStartField(float radius, float height);

//creates a billboard used for electric fences and chevrons
std::vector<sf::Vertex> createBillboard(sf::Vector2f start, sf::Vector2f end, float height, sf::Vector2f textureSize);

//creates a pillar placed on the end of fences
std::vector<sf::Vertex> createPylon(sf::Vector2f);

//creates a cylinder, assumes the top square of the texture (using its width)
//contains the shadow, and the rest is a vertical segment
std::vector<sf::Vertex> createCylinder(float radius, sf::Vector2f texSize, float height);

//creates the archway over the lap line
std::vector<sf::Vertex> createLapLine(/*sf::Vector2f texSize*/);