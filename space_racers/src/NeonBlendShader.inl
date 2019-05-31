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

//some of the below is taken from SFML Game Development

#pragma once

#include <string>

static const std::string ExtractFragment =
R"(
#version 120

uniform sampler2D u_texture;

const float threshold = 0.27;

void main()
{
    vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy);
    gl_FragColor = clamp((colour - threshold) / (1.0 - threshold), 0.0, 1.0);
})";

static const std::string BlendFragment =
R"(
#version 120

uniform sampler2D u_sceneTexture;
uniform sampler2D u_neonTexture;

float blend(float bg, float fg)
{
    //overlay mode
    return bg < 0.5 ? (2.0 * bg * fg) : (1.0 - 2.0 * (1.0 - bg) * (1.0 - fg));

    //screen mode
    //return 1.0 - ((1.0 - bg) * (1.0 - fg));

    //add
    //return bg + (fg * 2.0);
}

vec3 blend(vec3 background, vec3 foreground)
{
    return vec3(blend(background.r, foreground.r), blend(background.g, foreground.g), blend(background.b, foreground.b));
}

const float neonIntensity = 1.8;
const float sceneIntensity = 1.2;
const float neonSaturation = 2.8;
const float sceneSaturation = 2.0;

vec4 adjustSaturation(vec4 colour, float saturation)
{
    float grey = dot(colour.rgb, vec3(0.299, 0.587, 0.114));
    return vec4(mix(vec3(grey), colour.rgb, saturation), 1.0);
}

void main()
{
    vec4 sceneColour = texture2D(u_sceneTexture, gl_TexCoord[0].xy);
    vec4 neonColour = texture2D(u_neonTexture, gl_TexCoord[0].xy);

    sceneColour = adjustSaturation(sceneColour, sceneSaturation) * sceneIntensity;
    neonColour = adjustSaturation(neonColour, neonSaturation) * neonIntensity;

    sceneColour *= (1.0 - clamp(neonColour, 0.0, 1.0));

    gl_FragColor = sceneColour + neonColour;
})";