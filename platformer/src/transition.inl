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

static const std::string PixelateFrag = R"(
#version 120

uniform sampler2D u_texture;

uniform float u_time;
uniform vec2 u_screenSize;

void main()
{
	vec2 pixelSize = vec2((u_time * 50.0) + 1.0);

	vec2 coord = gl_TexCoord[0].xy;
	coord = floor(coord * u_screenSize / pixelSize) / u_screenSize * pixelSize;
	
	gl_FragColor = mix(texture2D(u_texture, coord), vec4(vec3(0.0), 1.0), u_time);
})";

static const std::string NoiseFrag = R"(
#version 120

uniform sampler2D u_texture;

uniform float u_time;

float rand(vec2 pos)
{
    return fract(sin(dot(pos, vec2(12.9898, 4.1414) + u_time)) * 43758.5453);
}

void main()
{
    vec4 noise = vec4(vec3(rand(floor((gl_FragCoord.xy / 4.0)))), 1.0);
    vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy);

    gl_FragColor = mix(colour, noise, u_time);

})";