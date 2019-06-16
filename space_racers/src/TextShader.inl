#pragma once

//modified version of https://www.shadertoy.com/view/Ms3XWH

#include <string>

static const std::string TextFragment =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_time;

const float noiseQuality = 250.0;
const float noiseIntensity = 0.0088;
const float offsetIntensity = 0.02;
const float colorOffsetIntensity = 1.3;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    float uvY = uv.y;
    uvY *= noiseQuality;
    uvY = float(int(uvY)) * (1.0 / noiseQuality);
    float noise = rand(vec2(u_time * 0.00001, uvY));
    uv.x += noise * noiseIntensity;

    vec2 offsetR = vec2(0.006 * sin(u_time), 0.0) * colorOffsetIntensity;
    vec2 offsetB = vec2(0.0073 * (cos(u_time * 0.97)), 0.0) * colorOffsetIntensity;

    float r = texture2D(u_texture, uv + offsetR).r * gl_Color.r;
    float g = texture2D(u_texture, uv).g * gl_Color.g;
    float b = texture2D(u_texture, uv + offsetB).b * gl_Color.b;

    float a = texture2D(u_texture, uv + offsetR).a;
    a += texture2D(u_texture, uv).a;
    a += texture2D(u_texture, uv + offsetB).a;
    a /= 3.0;

    gl_FragColor = vec4(r,g,b,a);
})";