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
static const std::string VehicleVertex =
R"(
#version 120

uniform vec3 u_camWorldPosition;
uniform mat4 u_modelMatrix;

const vec3 lightDirection = vec3(1.0, -1.0, 1.0);
const vec4 normal = vec4(0.0, 0.0, 1.0, 1.0);
const vec4 tangent = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 bitan = vec4(0.0, 1.0, 0.0, 1.0);

varying vec3 v_cameraTanDirection;
varying vec3 v_lightTanDirection;

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;

    //vec4 vertexWorldViewPosition = gl_ModelViewMatrix * gl_Vertex; //does this hold true with SFML's 4 vert 'optimisation'?
    //vec4 camWorldViewDirection = (/*gl_ModelViewMatrix */ vec4(u_camWorldPosition, 1.0)) - vertexWorldViewPosition;

    vec3 normalWorldView = (gl_ModelViewMatrix * normal).xyz;
    vec3 tanWorldView = (gl_ModelViewMatrix * tangent).xyz;
    vec3 bitanWorldView = (gl_ModelViewMatrix * bitan).xyz;

    mat3 TBN = transpose(mat3(tanWorldView, bitanWorldView, normalWorldView));
    v_cameraTanDirection = TBN * vec3(0.0, 0.0, 1.0);//camWorldViewDirection.xyz;
    v_lightTanDirection =  TBN * normalize(lightDirection);

})";

static const std::string VehicleFrag =
R"(
uniform sampler2D u_diffuseMap;
uniform sampler2D u_normalMap;

varying vec3 v_cameraTanDirection;
varying vec3 v_lightTanDirection;

void main()
{
    vec2 coord = gl_TexCoord[0].xy;
    vec4 diffuseColour = texture2D(u_diffuseMap, coord);
    vec3 normalColour = texture2D(u_normalMap, coord).rgb;

    vec3 normal = normalize(normalColour * 2.0 - 1.0);


    float colourAmount = max(dot(normal, normalize(v_lightTanDirection)), 0.0);
    
    vec3 specular = vec3(0.0);
    vec3 diffuse = vec3(0.0);

    vec3 halfVec = normalize(v_lightTanDirection + v_cameraTanDirection);
    float specAmount = max(dot(normal, halfVec), 0.0);
    specular = vec3(pow(specAmount, 120));
    diffuse = diffuseColour.rgb * colourAmount;

    gl_FragColor = vec4(specular + diffuse, diffuseColour.a);

})";