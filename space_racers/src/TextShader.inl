#pragma once

#include <string>

static const std::string TextFragment =
R"(
#version 120

uniform sampler2D u_texture;

void main()
{
    vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy);
    colour *= gl_Color;

    if(int(mod(gl_FragCoord.y, 2)) == 0)
    {
        colour.a *= 0.2;
    }

    gl_FragColor = colour;
})";