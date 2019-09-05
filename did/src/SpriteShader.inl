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

static const std::string SpriteVertex =
R"(

#define LIGHT_COUNT 4

uniform mat4 u_viewProjMat;
uniform mat4 u_modelMat;
uniform float u_cameraWorldPosition;

#if defined(LIGHTING)
uniform vec4 u_skyColour;
uniform vec3 u_lightWorldPosition[LIGHT_COUNT];

varying vec3 v_lightDir[LIGHT_COUNT];
varying vec3 v_lightingNormal;
#endif

varying vec3 v_worldPosition;

const float FadeDistance = 30.0;
const float ShadeDistance = 1200.0;
#if defined(CULL_FADE)
const float CullDistance = 690.0;
const float CullLength = 100.0;
#endif

void main()
{
    vec4 worldPos = u_modelMat * gl_Vertex;
    v_worldPosition = worldPos.xyz;

    gl_Position = u_viewProjMat * worldPos;//u_modelMat * gl_Vertex; //gl_ModelViewProjectionMatrix

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    gl_FrontColor = gl_Color;

    //fade alpha with distance to camera
    float dist = u_cameraWorldPosition - worldPos.z;
    
    gl_FrontColor.a *= clamp(dist / FadeDistance, 0.0, 1.0);

    //darken in the distance
    dist -= FadeDistance;
    float shade = 1.0 - clamp(dist / ShadeDistance, 0.0, 1.0);
    gl_FrontColor.rgb *= shade;
#if defined(CULL_FADE)
    //fade culled sprites just before they are culled
    float cull = 1.0 - clamp(((u_cameraWorldPosition - worldPos.z) - CullDistance) / CullLength, 0.0, 1.0);
    gl_FrontColor.a *= cull;
#endif
#if defined(LIGHTING)
    gl_FrontColor *= u_skyColour;

    for(int i = 0; i < LIGHT_COUNT; ++i)
    {
        v_lightDir[i] = u_lightWorldPosition[i] - worldPos.xyz;
    }
    v_lightingNormal = vec3(0.0, 0.0, 1.0);
#endif
})";

static const std::string SpriteFrag =
R"(
#version 120
#define LIGHT_COUNT 4

uniform sampler2D u_texture;
uniform vec3 u_lightColour[LIGHT_COUNT];
uniform float u_lightAmount;

varying vec3 v_lightDir[LIGHT_COUNT];
varying vec3 v_lightingNormal;
varying vec3 v_worldPosition;

//const vec3 normal = vec3(0.0, 0.0, 1.0);
const float LightRange = 1.0 / 64.0;


void main()
{
    if(v_worldPosition.y < 0) discard;

    vec4 diffuseColour = texture2D(u_texture, gl_TexCoord[0].xy);
    vec3 outColour = diffuseColour.rgb * gl_Color.rgb;

    for(int i = 0; i < LIGHT_COUNT; ++i)
    {
        vec3 lightDirection = v_lightDir[i];
        float diffAmount = max(dot(normalize(lightDirection), v_lightingNormal), 0.0);
        
        vec3 lDir = lightDirection * LightRange;
        float att = clamp(1.0 - dot(lDir, lDir), 0.0, 1.0);
        diffAmount *= att;

        //diffuseColour.rgb += u_lightColour[i] * diffAmount;
        outColour = mix(outColour, diffuseColour.rgb + u_lightColour[i], diffAmount * u_lightAmount);
    }
    

    gl_FragColor = vec4(outColour, diffuseColour.a * gl_Color.a);
})";

//NOTE if creating new shader variants remember to add them to the
//shader list passed to the 3D sprites system so cam world position is properly updated

static const std::string UnlitSpriteFrag =
R"(

#if defined(TEXTURED)
uniform sampler2D u_texture;
#endif

void main()
{
    gl_FragColor = gl_Color;//vec4(gl_Color.rgb, 1.0);
#if defined(TEXTURED)
    gl_FragColor *= texture2D(u_texture, gl_TexCoord[0].xy);
#endif
})";