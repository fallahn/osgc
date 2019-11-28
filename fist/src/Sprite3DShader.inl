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

//because we don't have direct access to vertex attribs
//we've fudged depth information into the alpha channel of the colour attrib
//as a multiplier of the z-depth set in the model matrix.
//vertex normal data is in the colour RGB

static const std::string SpriteVertexColoured =
R"(
#version 120

uniform mat4 u_viewProjMat;
uniform mat4 u_modelMat;

void main()
{
    vec4 worldPos = u_modelMat * gl_Vertex;
    worldPos.z *= gl_Color.a;

    gl_Position = u_viewProjMat * worldPos;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    gl_FrontColor = gl_Color;
})";

static const std::string SpriteVertexLighting =
R"(
#version 120

uniform mat4 u_viewProjMat;
uniform mat4 u_modelMat;
uniform vec3 u_pointlightWorldPosition;

varying vec3 v_normal;
varying vec3 v_pointlightDirection;

void main()
{
    vec4 worldPos = u_modelMat * gl_Vertex;
    worldPos.z *= gl_Color.a * length(vec3(u_modelMat[2][0], u_modelMat[2][1], u_modelMat[2][2]));

    gl_Position = u_viewProjMat * worldPos;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    vec4 normal = vec4(gl_Color.rgb * 2.0 - 1.0, 0.0);
    v_normal = normalize((u_modelMat * normal).xyz);

    v_pointlightDirection = u_pointlightWorldPosition - worldPos.xyz;
})";

static const std::string SpriteVertexOutline =
R"(
#version 120

uniform mat4 u_viewProjMat;
uniform mat4 u_modelMat;

//varying vec3 v_normal;

void main()
{
    vec4 worldPos = u_modelMat * gl_Vertex;
    worldPos.z *= gl_Color.a * length(vec3(u_modelMat[2][0], u_modelMat[2][1], u_modelMat[2][2]));

    vec4 normal = vec4(gl_Color.rgb * 2.0 - 1.0, 0.0);
    normal.xyz = normalize((u_modelMat * normal).xyz);
    worldPos.xyz += normal.xyz * 2.0;

    gl_Position = u_viewProjMat * worldPos;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    //v_normal = normal;
})";

static const std::string SpriteOutlineFrag =
R"(
#version 120

void main()
{
    gl_FragColor = vec4(0.2, 1.0, 0.1, 1.0);
})";

//the tangent calc makes some assumptions about the normals
//as such will only work for specific wall geometry.
static const std::string SpriteVertexWalls =
R"(
#version 120
#define SPRITE_COUNT 2

uniform mat4 u_projMat;
uniform mat4 u_viewMat;
uniform mat4 u_modelMat;

uniform vec3 u_pointlightWorldPosition;
uniform vec3 u_spriteShadowWorldPosition[SPRITE_COUNT];

varying vec3 v_pointlightDirection;
varying vec3 v_spriteShadowDirection[SPRITE_COUNT];
varying vec4 v_viewPosition;

varying mat3 v_tbn;

void main()
{
    vec4 worldPos = u_modelMat * gl_Vertex;
    worldPos.z *= gl_Color.a;

    v_viewPosition = u_viewMat * worldPos;
    gl_Position = u_projMat * u_viewMat * worldPos;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    vec4 normal = normalize(vec4(gl_Color.rgb * 2.0 - 1.0, 1.0));
    vec4 tan = vec4(0.0);
    vec4 bit = vec4(0.0);

    if(abs(normal.x) > 0.3)
    {
        tan.z = -normal.x;
        bit.y = -1.0;
    }
    else if(abs(normal.y) > 0.3)
    {
        bit.z = -normal.y;
        tan.x = normal.y;
    }
    else if(abs(normal.z) > 0.3)
    {
        tan.x = normal.z;
        bit.y = -1.0;
    }

    normal = normalize(u_modelMat * normal);
    tan = normalize(u_modelMat * tan);
    bit = normalize(u_modelMat * bit);

    v_tbn = (mat3(tan.xyz, bit.xyz, normal.xyz));

    v_pointlightDirection = u_pointlightWorldPosition - worldPos.xyz;
    for(int i = 0; i < SPRITE_COUNT; ++i)
    {
        v_spriteShadowDirection[i] = u_spriteShadowWorldPosition[i] - worldPos.xyz;
    }
})";

static const std::string SpriteFragmentWalls =
R"(
#version 120
#define SPRITE_COUNT 2

uniform sampler2D u_texture;
uniform sampler2D u_normalMap;
uniform sampler2D u_lightmap;

uniform vec3 u_skylightColour;
uniform float u_skylightAmount;

uniform vec3 u_roomlightColour;
uniform float u_roomlightAmount;

uniform vec3 u_lightDirection;

varying vec3 v_pointlightDirection;
varying vec3 v_spriteShadowDirection[SPRITE_COUNT];
varying vec4 v_viewPosition;

varying mat3 v_tbn;

const float FogNear = 2880.0;
const float FogFar = 4800.0;

const float AmbientAmount = 0.3;
const float LightRange = 1.0 / 540.0;
const float ShadowRange = 128.0;

void main()
{
    vec4 baseColour = texture2D(u_texture, gl_TexCoord[0].xy);
    if(baseColour.a < 0.1)
    {
        discard;
    }

    baseColour.rgb *= texture2D(u_lightmap, gl_TexCoord[0].xy).rgb;

    vec3 normal = texture2D(u_normalMap, gl_TexCoord[0].xy).rgb * 2.0 - 1.0;
    normal = normalize(normalize(normal) * v_tbn);

    vec3 lightDir = normalize(vec3(-0.1, 0.2, 1.0));

    float diffuseAmount = max(dot(lightDir, normal), 0.5);

    vec3 ambient = baseColour.rgb * AmbientAmount;
    vec3 diffuse = baseColour.rgb * diffuseAmount;

    vec3 skyColour = (ambient + diffuse) * (u_skylightColour * u_skylightAmount);


    //point light
    diffuseAmount = max(dot(normalize(v_pointlightDirection), normal), 0.0);

    vec3 lDir = v_pointlightDirection * LightRange;
    float att = clamp(1.0 - dot(lDir, lDir), 0.0, 1.0);
    diffuseAmount *= att;

    diffuse = baseColour.rgb * diffuseAmount;

    vec3 pointColour = (ambient + diffuse) * (u_roomlightColour * u_roomlightAmount);

    gl_FragColor = vec4((skyColour + pointColour), baseColour.a);

    //sprite shadows
    for(int i = 0; i < SPRITE_COUNT; ++i)
    {
        vec3 sDir = v_spriteShadowDirection[i];
        att = clamp(dot(sDir, sDir) / (ShadowRange * ShadowRange), 0.0, 0.18) + 0.82;
        att *= att;
        gl_FragColor.rgb *= att;
    }

    //distance fogging
    float distance = length(v_viewPosition);
    float fogAmount = (FogFar - distance) / (FogFar - FogNear);
    fogAmount = clamp(fogAmount, 0.0, 1.0 );

    gl_FragColor.rgb = mix(vec3(0.0), gl_FragColor.rgb, fogAmount);

})";

static const std::string SpriteFragmentTextured =
R"(
#version 120

uniform sampler2D u_texture;

uniform vec3 u_skylightColour;
uniform float u_skylightAmount;

uniform vec3 u_roomlightColour;
uniform float u_roomlightAmount;

varying vec3 v_normal;
varying vec3 v_pointlightDirection;

//const float LightRadius = 480.0 * 480.0;
const float LightRange = 1.0 / 480.0;
const float AmbientAmount = 0.1;

void main()
{
    vec4 baseColour = texture2D(u_texture, gl_TexCoord[0].xy);
    if(baseColour.a < 0.1)
    {
        discard;
    }

    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(vec3(0.2, 0.2, 1.0));

    float diffuseAmount = max(dot(lightDir, normal), 0.5);

    vec3 ambient = baseColour.rgb * AmbientAmount;
    vec3 diffuse = baseColour.rgb * diffuseAmount;

    vec3 skyColour = (ambient + diffuse) * (u_skylightColour * u_skylightAmount);


    //point light
    diffuseAmount = max(dot(normalize(v_pointlightDirection), normal), 0.0);

    vec3 lDir = v_pointlightDirection * LightRange;
    float att = clamp(1.0 - dot(lDir, lDir), 0.0, 1.0);
    diffuseAmount *= att;

    diffuse = baseColour.rgb * diffuseAmount;

    vec3 pointColour = (ambient + diffuse) * (u_roomlightColour * u_roomlightAmount);

    gl_FragColor = vec4((skyColour + pointColour), baseColour.a);
})";

static const std::string SpriteFragmentColoured =
R"(
#version 120

void main()
{
    gl_FragColor = vec4(gl_Color.rgb, 1.0);
})";