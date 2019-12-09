/*********************************************************************
(c) Matt Marchant 2019

xygineXT - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#pragma once

#include <string>

/*
Random function from
https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
*/
static const std::string NoiseFragment = R"(
#version 120

uniform float u_time;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    gl_FragColor = vec4(vec3(rand((gl_TexCoord[0].xy) + vec2(u_time))), 1.0);
})";


/*
Shader from
https://www.shadertoy.com/view/3dBSRD
*/
static const std::string ScanlineFrag = R"(
#version 120

uniform sampler2D u_texture;
uniform float u_time;

const float density = 1.0;
const float opacityScanline = 0.4;
const float opacityNoise = 0.2;
const float flickering = 0.03;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float blend(const float x, const float y)
{
	return (x < 0.5) ? (2.0 * x * y) : (1.0 - 2.0 * (1.0 - x) * (1.0 - y));
}

vec3 blend(const vec3 x, const vec3 y, const float opacity) 
{
	vec3 z = vec3(blend(x.r, y.r), blend(x.g, y.g), blend(x.b, y.b));
	return z * opacity + x * (1.0 - opacity);
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;
    vec3 col = texture2D(u_texture, uv).rgb;
    
    float count = 1080.0 * density;
    vec2 sl = vec2(sin(uv.y * count), cos(uv.y * count));
	vec3 scanlines = vec3(sl.x, sl.y, sl.x);

    col += col * scanlines * opacityScanline;
    col += col * vec3(rand(uv*u_time)) * opacityNoise;
    col += col * sin(110.0*u_time) * flickering;

    gl_FragColor = vec4(col,1.0);
})";

static const std::string ScanlineClassicFrag = R"(
#version 120

uniform sampler2D u_texture;

void main()
{
    float amount = 1.0;
    if(mod(floor(gl_FragCoord.y), 2.0) == 0)
    {
        amount = 0.75;
    }

    gl_FragColor = vec4(vec3(texture2D(u_texture, gl_TexCoord[0].xy).rgb * amount), 1.0);
})";