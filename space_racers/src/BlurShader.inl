//lifted from xygine source, but probably originally from the SFML Game Development book

#pragma once

#include <string>

static const std::string BlurFragment =
R"(#version 120
uniform sampler2D u_texture;
uniform vec2 u_offset;

void main()
{
    vec2 texCoords = gl_TexCoord[0].xy;
    vec4 colour = vec4(0.0);
    colour += texture2D(u_texture, texCoords - 4.0 * u_offset) * 0.0162162162;
    colour += texture2D(u_texture, texCoords - 3.0 * u_offset) * 0.0540540541;
    colour += texture2D(u_texture, texCoords - 2.0 * u_offset) * 0.1216216216;
    colour += texture2D(u_texture, texCoords - u_offset) * 0.1945945946;
    colour += texture2D(u_texture, texCoords) * 0.2270270270;
    colour += texture2D(u_texture, texCoords + u_offset) * 0.1945945946;
    colour += texture2D(u_texture, texCoords + 2.0 * u_offset) * 0.1216216216;
    colour += texture2D(u_texture, texCoords + 3.0 * u_offset) * 0.0540540541;
    colour += texture2D(u_texture, texCoords + 4.0 * u_offset) * 0.0162162162;
    gl_FragColor = colour;
})";