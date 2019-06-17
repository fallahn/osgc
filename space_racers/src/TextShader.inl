#pragma once

#include <string>

static const std::string TextFragment =
R"(
#version 120

uniform sampler2D u_texture;

const float maxOffset = 1.0 / 450.0; //abberation sample offset

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    vec2 offset = vec2((maxOffset / 2.0) - (uv.x * maxOffset), (maxOffset / 2.0) - (uv.y * maxOffset));

    float r = texture2D(u_texture, uv + offset).r;// * gl_Color.r;
    float g = texture2D(u_texture, uv).g;// * gl_Color.g;
    float b = texture2D(u_texture, uv - offset).b;// * gl_Color.b;

    float a = texture2D(u_texture, uv + offset).a;
    a += texture2D(u_texture, uv).a;
    a += texture2D(u_texture, uv - offset).a;
    a /= 3.0;

    gl_FragColor = vec4(r,g,b,a * gl_Color.a);
})";