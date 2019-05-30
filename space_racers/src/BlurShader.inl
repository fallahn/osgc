//lifted from xygine source, but probably originally from the SFML Game Development book

#pragma once

#include <string>

static const std::string BlurFragment =
"#version 120\n" \
"uniform sampler2D u_texture;\n" \
"uniform vec2 u_offset;\n" \

"void main()\n" \
"{\n " \
"    vec2 texCoords = gl_TexCoord[0].xy;\n" \
"    vec4 colour = vec4(0.0);\n" \
"    colour += texture2D(u_texture, texCoords - 4.0 * u_offset) * 0.0162162162;\n" \
"    colour += texture2D(u_texture, texCoords - 3.0 * u_offset) * 0.0540540541;\n" \
"    colour += texture2D(u_texture, texCoords - 2.0 * u_offset) * 0.1216216216;\n" \
"    colour += texture2D(u_texture, texCoords - u_offset) * 0.1945945946;\n" \
"    colour += texture2D(u_texture, texCoords) * 0.2270270270;\n" \
"    colour += texture2D(u_texture, texCoords + u_offset) * 0.1945945946;\n" \
"    colour += texture2D(u_texture, texCoords + 2.0 * u_offset) * 0.1216216216;\n" \
"    colour += texture2D(u_texture, texCoords + 3.0 * u_offset) * 0.0540540541;\n" \
"    colour += texture2D(u_texture, texCoords + 4.0 * u_offset) * 0.0162162162;\n" \
"    gl_FragColor = colour;\n" \
"}";