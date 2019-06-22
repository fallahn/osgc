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

static const std::string  ShieldFragment =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_time;

const vec3 tintColour = vec3(0.4, 0.0, 1.0);

void main()
{
    vec2 coord = gl_TexCoord[0].xy;

    vec2 centre = -1.0 + 2.0 * coord;
    float radius = sqrt(dot(centre, centre));
    if (radius < 1.0)
    {
        float offsetAmount = (1.0 - sqrt(1.0 - radius)) / (radius);

        vec2 uv = centre * offsetAmount + u_time;

        vec3 colour = texture2D(u_texture, uv).rgb + (tintColour * pow(offsetAmount, 4.0));
        gl_FragColor = vec4(colour, gl_Color.a);
    }
    else
    {
        gl_FragColor = vec4(0.0);
    }
})";