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

/*
Shaders used when rendering the island plane
*/

static const std::string MoonFrag2 =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_dayNumber;
uniform vec4 u_skyColour;

const float DaysPerCycle = 28.0;
const float Tau = 6.28;

void main()
{
    float dayNumber = (mod(u_dayNumber, DaysPerCycle) / DaysPerCycle) * Tau;
    vec3 light = normalize(vec3(sin(dayNumber), 0.5, cos(dayNumber)));

    const float radius = 0.5; //texture must be square fot this to work
    vec3 position = vec3((gl_TexCoord[0].x - 0.5), gl_TexCoord[0].y - 0.5, 0.0);
    float z = sqrt(radius * radius - position.x * position.x - position.y * position.y);
    vec3 normal = normalize(vec3(position.x, position.y, z));
    float diffuse = max(0.05, dot(normal, light));

    vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy);
    vec4 colourDark = colour;
    colourDark.rgb *= u_skyColour.rgb * 0.7;
    //colourDark.a *= (1.0 - (u_skyColour.r + u_skyColour.g + u_skyColour.b) / 3.0);
    
    colour = mix(colourDark, colour, diffuse);
    gl_FragColor = colour;
})";

static const std::string MoonFrag =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_dayNumber;

const float DaysPerCycle = 28.0;
const float HalfCycle = 14.0;

const float ShadowHardness = 16.0;
const float ShadowDarkness = 0.3;
const float PI = 3.142;

void main()
{
    float currentDay = mod(u_dayNumber, DaysPerCycle);
    float shadowWidth = mod(currentDay, HalfCycle) / HalfCycle;
    float shadowOffset = 0.0;
    if(currentDay > HalfCycle)
    {
        shadowOffset = shadowWidth;
        shadowWidth = 1.0 - shadowWidth;
    }

    vec2 coord = gl_TexCoord[0].xy;
    vec4 colour = texture2D(u_texture, coord);

    float sinVal = sin(coord.y * PI);
    float curve = sinVal * (0.5 - (shadowOffset + shadowWidth)) * 0.5;

    coord.x *= 2.0; //because the texture is actually twice as wide
    float shadow = clamp(((coord.x + curve) - (shadowOffset + shadowWidth)) * ShadowHardness, 0.0, 1.0);

    //recalc curve
    curve = sinVal * (0.5 - shadowOffset) * 0.5;
    shadow += (1.0 - clamp(((coord.x + curve) - shadowOffset) * ShadowHardness, 0.0, 1.0));

    shadow = clamp(shadow, ShadowDarkness, 1.0);
    {
        colour.rgb *= shadow;
    }

    gl_FragColor = colour;
})";

static const std::string SkyFrag =
R"(
#version 120

uniform sampler2D u_texture;
uniform float u_time;
uniform vec4 u_skyColour;

void main()
{
    vec2 coord = gl_TexCoord[0].xy;
    coord.x -= u_time;

    gl_FragColor = texture2D(u_texture, coord) * u_skyColour * gl_Color;

})";

static const std::string GroundVertLit =
R"(
#version 120
#define LIGHT_COUNT 4

uniform mat4 u_view;
uniform mat4 u_viewProjection;
//don't need model because terrain is literally positioned

uniform vec3 u_cameraWorldPosition;
uniform vec4 u_skyColour;
uniform vec3 u_lightWorldPosition[LIGHT_COUNT];

const float MaxFadeDistance = 2048.0;
const mat3 TangentMatrix = mat3(1.0,0.0,0.0,
                                0.0,0.0,-1.0,
                                0.0,1.0,0.0); //ground plane is fixed so we can optimise with const matrix

varying vec3 v_lightDir[LIGHT_COUNT];
varying vec3 v_lightingNormal;

void main()
{
    vec4 vertex = vec4(gl_Vertex.x, gl_Vertex.z, gl_Vertex.y, gl_Vertex.w);

    gl_Position = u_viewProjection * vertex;
    //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

    gl_FrontColor = gl_Color * u_skyColour;

    //see note about no world mat
    float dist = u_cameraWorldPosition.z - vertex.z;
    float fade = 1.0 - clamp(dist / MaxFadeDistance, 0.0, 1.0);
    gl_FrontColor.rgb *= fade;

    for(int i = 0; i < LIGHT_COUNT; ++i)
    {
        v_lightDir[i] = u_lightWorldPosition[i] - vertex.xyz;
    }
    v_lightingNormal = vec3(0.0, 1.0, 0.0);
})";

static const std::string GroundFrag =
R"(

#version 120
#define LIGHT_COUNT 4

uniform sampler2D u_diffuseTexture;
uniform sampler2D u_normalTexture;

uniform vec3 u_sunDirection;
uniform vec3 u_lightColour[LIGHT_COUNT];
uniform float u_lightAmount;
uniform float u_normalUVOffset;

varying vec3 v_lightDir[LIGHT_COUNT];

const float LightRange = 1.0 / 64.0;
const float LightRadius = 64.0 * 64.0;

void main()
{
    vec2 normalCoord = gl_TexCoord[0].xy;
    normalCoord.y += u_normalUVOffset;
    vec3 normalColour = texture2D(u_normalTexture, normalCoord).rgb;
    vec3 normal = normalize(normalColour * 2.0 - 1.0);

    vec4 baseColour = texture2D(u_diffuseTexture, gl_TexCoord[0].xy);
    vec3 ambient = baseColour.rgb * 0.1;

    //sun light
    vec3 lightDirection = normalize(u_sunDirection);
    float diffAmount = max(dot(lightDirection, normal), 0.3);
    vec3 diffuse = diffAmount * baseColour.rgb * gl_Color.rgb;
    
    //point lights
    for(int i = 0; i < LIGHT_COUNT; ++i)
    {
        diffAmount = max(dot(normalize(v_lightDir[i].xzy), normal), 0.0);

        vec3 lDir = v_lightDir[i] * LightRange;
        float att = clamp(1.0 - dot(lDir, lDir), 0.0, 1.0);
        //float dist = length(v_lightDir[i]);
        //float att = clamp(1.0 - (dist * dist) / LightRadius, 0.0, 1.0);
        //att *= att;
        diffAmount *= att;

        diffuse = mix(diffuse, baseColour.rgb + u_lightColour[i], diffAmount * u_lightAmount);
    }
    
    gl_FragColor = vec4(ambient + diffuse, baseColour.a);//vec4(normal, 1.0);//
})";

static const std::string SeaFrag =
R"(
#version 120

uniform float u_time;
uniform sampler2D u_texture;
//uniform sampler2D u_seabedTexture;
uniform vec2 u_textureOffset;

vec3 mod289(vec3 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
     return mod289(((x * 34.0) + 1.0) * x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
{ 
  const vec2  C = vec2(1.0 / 6.0, 1.0 / 3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

  //First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

  //Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

  //Permutations
  i = mod289(i); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

  //Gradients: 7x7 points over a square, mapped onto an octahedron.
  //The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_);    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

  //Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

  //Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3)));
}

vec3 hsv(float h, float s, float v)
{
  return mix(vec3(1.0), clamp((abs(fract(
    h + vec3(3.0, 2.0, 1.0) / 3.0) * 6.0 - 3.0) - 1.0), 0.0, 1.0), s) * v;
}

float s_step(float edge0, float edge1, float x)
{
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

float rand(vec2 uv)
{
    return fract(sin(dot(uv.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy + u_textureOffset;
    vec3 pos = vec3(uv * vec2(3.0, 1.0) - vec2(u_time * 0.03, 0.0), u_time * 0.01);
   
    float n = s_step(0.6, 1.0, snoise(pos * 60.0)) * 10.0;
    vec3 col = hsv(n * 0.2 + 0.7 , 0.4 , 1.0);
    
    vec4 baseColour = texture2D(u_texture, uv);// + vec2(u_time * 0.005));
    float noiseValue = mod(rand(uv) + sin((u_time * 2.0)), 1.0);
    baseColour.rgb *= noiseValue;
    //vec4 groundColour = texture2D(u_seabedTexture, uv * 48.0);
    
    gl_FragColor =  vec4(col * vec3(n) + baseColour.rgb, 1.0);
})";