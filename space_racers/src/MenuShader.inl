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

static const std::string StarsFragment =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_time;
uniform float u_speed;

void main()
{
    vec2 coord = gl_TexCoord[0].xy;
    coord.x -= u_time * u_speed;
    coord.y -= u_time * (u_speed / 2.0);

    gl_FragColor = texture2D(u_texture, coord);
})";

static const std::string LightbarFragment =
R"(
#version 120

uniform sampler2D u_texture;

float blend(float bg, float fg)
{
    //overlay mode
    return bg < 0.5 ? (2.0 * bg * fg) : (1.0 - 2.0 * (1.0 - bg) * (1.0 - fg));
}

void main()
{
    vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy);

    colour.r = blend(colour.r, gl_Color.r);
    colour.g = blend(colour.g, gl_Color.g);
    colour.b = blend(colour.b, gl_Color.b);

    gl_FragColor = colour;
})";