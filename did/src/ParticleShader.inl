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

const std::string ParticleVertex = R"(
        #version 120    
        
        uniform float u_screenScale;
        uniform mat4 u_viewProjection;

        varying mat2 v_rotation;
        
        void main()
        {
            vec4 rotScale = gl_TextureMatrix[0] * gl_MultiTexCoord0; //we've shanghaid the text coords to hold rotation and scale
            vec2 rot = vec2(sin(rotScale.x), cos(rotScale.x));
            v_rotation[0] = vec2(rot.y, -rot.x);
            v_rotation[1] = rot;
            gl_FrontColor = gl_Color;

            vec4 position = gl_Vertex.xyzw;
            gl_Position = u_viewProjection * position;
            //gl_Position = gl_ModelViewProjectionMatrix * position;
            gl_PointSize = rotScale.y * u_screenScale;
        })";

const std::string ParticleFragment = R"(
        #version 120
        uniform sampler2D u_texture;
        
        varying mat2 v_rotation;
        void main()
        {
            vec2 texCoord = v_rotation * (gl_PointCoord - vec2(0.5));
            gl_FragColor = gl_Color * texture2D(u_texture, texCoord + vec2(0.5));
        })";