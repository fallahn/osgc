
// Copyright (c) 2012 by Mickaël Pointier. 
// This work is made available under the terms of the Creative Commons Attribution-ShareAlike 3.0 Unported license,
// http://creativecommons.org/licenses/by-sa/3.0/.
// modified to work with multiple tile sizes and tile set sizes

#include <string>

static const std::string tilemapFrag = R"(
#version 120

uniform sampler2D u_indexMap;
uniform sampler2D u_tileSet;
uniform vec2 u_indexSize; //resolution of index map aka map tile count x/y
uniform vec2 u_tileCount; //tile count in tileset

//const vec2 tileSize = vec2(16.0);
const float epsilon = 0.000005; //try reduce rounding errors

void main()
{
  vec2 tilePos = gl_TexCoord[0].xy;// / tileSize;

  float index = floor(texture2D(u_indexMap, tilePos).r * 256.0);

  //tmx maps start at index 1, 0 is no tile
  if(index == 0)
  {
    gl_FragColor = vec4(0.0);
  }
  else
  {
    index -= 1.0;

    vec2 normalisedTileSize = vec2(1.0) / u_tileCount;
    vec2 baseTilePos = normalisedTileSize * floor(vec2(mod(index + epsilon, u_tileCount.x), (index / u_tileCount.x) + epsilon)); 

    vec2 internalPos = normalisedTileSize * mod(gl_TexCoord[0].xy * u_indexSize /** tileSize*/, vec2(1.00001));

    gl_FragColor = texture2D(u_tileSet, baseTilePos + internalPos);
  }
})";

//alt version which may or may not reduce tearing. I haven't decided.
static const std::string tilemapFrag2 = R"(
#version 120

uniform sampler2D u_indexMap;
uniform sampler2D u_tileSet;
uniform vec2 u_indexSize; //resolution of index map aka map tile count x/y
uniform vec2 u_tileCount; //tile count in tileset
uniform vec2 u_tileSize;

const float epsilon = 0.000005; //try reduce rounding errors

void main()
{
    float index = floor(texture2D(u_indexMap, gl_TexCoord[0].xy).r * 256.0);

    //tmx maps start at index 1, 0 is no tile
    if(index == 0)
    {
        gl_FragColor = vec4(0.0);
    }
    else
    {
        index -= 1.0;

        vec2 position = vec2(mod(index + epsilon, u_tileCount.x), floor((index / u_tileCount.x) + epsilon)) / u_tileCount;


        vec2 texelSize = vec2(1.0) / u_indexSize;
        vec2 offset = mod(gl_TexCoord[0].xy, texelSize);
        vec2 ratio = offset / texelSize;
        offset = ratio * (1.0 / u_tileSize);
        offset *= u_tileSize / u_tileCount;

        gl_FragColor = texture2D(u_tileSet, position + offset);
    }
})";