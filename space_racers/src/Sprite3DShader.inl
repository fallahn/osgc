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

static const std::string SpriteVertex =
R"(
#version 120

uniform mat4 u_viewProjMat;
uniform mat4 u_modelMat;

void main()
{
    vec4 worldPos = u_modelMat * gl_Vertex;
    //v_worldPosition = worldPos.xyz;

    gl_Position = u_viewProjMat * worldPos;//u_viewProjMat * worldPos;//u_modelMat * gl_Vertex; //gl_ModelViewProjectionMatrix

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    gl_FrontColor = gl_Color;
})";

static const std::string SpriteFragment =
R"(
#version 120

uniform sampler2D u_texture;

void main()
{
    gl_FragColor = texture2D(u_texture, gl_TexCoord[0].xy);
})";