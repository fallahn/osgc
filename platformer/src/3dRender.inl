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

#include <string>

//we'll assume layers are literally placed
//and therefore have no world transform

static const std::string Layer3DVertex = R"(

#version 120

uniform mat4 u_viewProjectionMatrix;
uniform float u_depth;

void main()
{
    vec4 position = gl_Vertex;
    position.z = u_depth;

    gl_Position = u_viewProjectionMatrix * position;

    //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    gl_FrontColor = gl_Color;
})";

static const std::string TileEdgeVert = R"(
#version 120

uniform mat4 u_viewProjectionMatrix;

void main()
{
    vec4 position = gl_Vertex;
    position.z = gl_Color.a * 255.0 - 128.0;
    gl_Position = u_viewProjectionMatrix * position;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    gl_FrontColor = vec4(gl_Color.rgb * (0.1 + (0.9 * gl_Color.a)), 1.0);
})";

static const std::string TileEdgeFrag = R"(
#version 120

uniform sampler2D u_texture;

void main()
{
    vec2 coord = gl_TexCoord[0].xy;
    vec4 colour = texture2D(u_texture, coord);
    if(colour.a < 0.2)
    {
        discard;
    }

    gl_FragColor = gl_Color * colour;
})";

static const std::string Sprite3DVertex =
R"(
#version 120

uniform mat4 u_viewProjectionMatrix;
uniform mat4 u_modelMat;

uniform float u_zOffset;

void main()
{
    vec4 worldPos = u_modelMat * gl_Vertex;
    worldPos.z *= gl_Color.a;
    worldPos.z += u_zOffset;

    gl_Position = u_viewProjectionMatrix * worldPos;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    gl_FrontColor = gl_Color;
})";

static const std::string Sprite3DFrag = R"(
#version 120

uniform sampler2D u_texture;

void main()
{
    vec2 coord = gl_TexCoord[0].xy;
    vec4 colour = texture2D(u_texture, coord);


    gl_FragColor = colour;
})";