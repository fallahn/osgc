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

//this is basically a normal map shader, but we make
//some assumptions because we're 2D. The main one being
//that the normal map always faces along the z-axis so
//placing the light and camera directions in tangent space
//requires only transforming them by the inverse of the model
//matrix.

/*
A problem with SFML is that it 'optimises' drawing less than 5
vertices by pre-transforming them. This means that gl_Vertex is
*already* in world coords, and gl_ModelViewMatrix is an identity
matrix, making it effectively useless.

*/

static const std::string VehicleVertex =
R"(
#version 120

//uniform vec3 u_camWorldPosition;
uniform mat4 u_lightRotationMatrix; //rotates the light source the opposite way to the sprite - this is because the only relative transform is z rotation

const vec3 lightDirection = normalize(vec3(1.0, 0.0, 2.0));
const vec4 normal = vec4(0.0, 0.0, 1.0, 1.0);
const vec4 tangent = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 bitan = vec4(0.0, 1.0, 0.0, 1.0);

const vec3 lightPosition = vec3(200.0, 200.0, 3000.0);

varying vec3 v_cameraTanDirection;
varying vec3 v_lightTanDirection;

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;

    v_cameraTanDirection = vec3(0.0,0.0,1.0);//u_camWorldPosition - gl_Vertex.xyz;
    v_lightTanDirection = (u_lightRotationMatrix * vec4(lightDirection, 1.0)).xyz;

})";

static const std::string VehicleFrag =
R"(
uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_specularMap;
uniform sampler2D u_neonMap;
uniform vec4 u_neonColour;

varying vec3 v_cameraTanDirection;
varying vec3 v_lightTanDirection;

float BlendOverlay(float base, float blend)
{
    return (base < 0.5) ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend));
}

void main()
{
    vec2 coord = gl_TexCoord[0].xy;
    vec4 diffuseColour = texture2D(u_diffuseMap, coord);
    vec3 normalColour = texture2D(u_normalMap, coord).rgb;

    vec3 normal = normalize(normalColour * 2.0 - 1.0);


    float colourAmount = max(dot(normal, normalize(v_lightTanDirection)), 0.0);
    float specularMultiplier = texture2D(u_specularMap, coord).r;
    
    vec3 specular = vec3(0.0);
    vec3 diffuse = vec3(0.0);

    vec3 halfVec = normalize(v_lightTanDirection + v_cameraTanDirection);
    float specAmount = max(dot(normal, halfVec), 0.0);
    specular = vec3(pow(specAmount, 120.0)) * specularMultiplier;
    diffuse = diffuseColour.rgb * colourAmount;

    vec4 neonSample = texture2D(u_neonMap, coord);
    vec3 neon = vec3(BlendOverlay(neonSample.r, u_neonColour.r), BlendOverlay(neonSample.g, u_neonColour.g), BlendOverlay(neonSample.b, u_neonColour.b));

    gl_FragColor = vec4(specular + diffuse, diffuseColour.a);
    gl_FragColor.rgb = mix(gl_FragColor.rgb, neon, dot(neonSample.rgb, vec3(0.299, 0.587, 0.114))); //uses neon map brightness as blend amount
})";

static const std::string VehicleTrail =
R"(
uniform sampler2D u_texture;

void main()
{
    gl_FragColor = texture2D(u_texture, gl_TexCoord[0].xy) + gl_Color;
})";