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

static const std::string MonitorFragment =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_time;

const float scanlineCount = 125.0 * 6.28; //line count * approx tau
const vec3 tintColour = vec3(0.42, 0.4, 0.96);

void main()
{
    vec2 coord = gl_TexCoord[0].xy;
    vec4 colour = texture2D(u_texture, coord);

    float scanline = sin((coord.y + u_time) * scanlineCount);
    scanline += 1.0;
    scanline /= 2.0;
    scanline *= 0.8;
    scanline += 0.2;
    colour *= scanline;

    //colour.rgb = vec3(1.0) - colour.rgb;
    colour.rgb *= tintColour;

    gl_FragColor = colour;
})";