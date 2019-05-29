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

static const std::string TrackFragment =
R"(
#version 120

uniform sampler2D u_texture;
uniform sampler2D u_normalMap;

const float distortionAmount = 0.001; //this should be approx 2/vec2(1920,1080)

void main()
{
    vec2 coords = gl_TexCoord[0].xy;
    vec3 offset = texture2D(u_normalMap, coords).rgb; //not a valid vector without 3rd component!
    offset = normalize(offset * 2.0 - 1.0);

    offset *= distortionAmount;

    //gl_FragColor.rg = offset.rg;
    //gl_FragColor.a = 1.0;
    gl_FragColor = texture2D(u_texture, coords + offset.rg);
})";