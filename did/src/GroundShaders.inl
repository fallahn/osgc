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

static const std::string GroundVert =
R"(
            #version 120

            uniform mat4 u_viewProjectionMatrix;

            void main()
            {
                vec4 vertex = gl_Vertex.xzyw;
                gl_Position = u_viewProjectionMatrix * vertex;
                gl_FrontColor = gl_Color;
                gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
            })";

static const std::string CompassFrag =
R"(
            #version 120
            uniform sampler2D u_texture;

            void main()
            {
                gl_FragColor = gl_Color * texture2D(u_texture, gl_TexCoord[0].xy);
            })";

static const std::string ShadowFrag =
R"(
            #version 120

            uniform sampler2D u_texture;
            uniform float u_amount;

            void main()
            {
                vec4 colour = texture2D(u_texture, gl_TexCoord[0].xy);
                colour = vec4(1.0 - colour.a);
                colour.a = 1.0;
                colour.rgb = mix(colour.rgb, vec3(1.0), gl_Color.rgb);

                colour.rgb = mix(vec3(1.0), colour.rgb, vec3(u_amount));

                gl_FragColor = colour;
            })";