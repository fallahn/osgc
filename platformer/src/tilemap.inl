
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

void main()
{
  vec2 tilePos = gl_TexCoord[0].xy;

  float index = floor(texture2D(u_indexMap, tilePos).r * 256.0);

  vec2 normalisedTileSize = vec2(1.0) / u_tileCount;
  vec2 baseTilePos = normalisedTileSize * floor(vec2(mod(index,u_tileCount.x),index / u_tileCount.x)); 

  vec2 internalPos = normalisedTileSize * mod(gl_TexCoord[0].xy * u_indexSize, vec2(1.0));

  gl_FragColor = texture2D(u_tileSet, baseTilePos + internalPos);
})";