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

namespace Sunset
{
    const std::string Fragment = 
    R"(
        #version 120

        uniform sampler2D u_texture;
        uniform float u_time;
        uniform float u_lightness;

        //const float Lightness = 0.4;
        const float Frequency = 1500.0;
        const float Amplitude = 0.0025;

        const float FadeStart = 0.56;
        const float FadeEnd = 0.7;

        void main()
        {
            vec2 coord = gl_TexCoord[0].xy;
            float gradient = 1.0 - clamp((coord.y - FadeStart) / (FadeEnd - FadeStart), 0.0, 1.0);
            //coord.x = 0.5 + ((coord.x - 0.5) * (0.4 + (gradient - 0.4))); //why do I feel like this should be an exponential curve?

            float time = u_time * (1.0 + (5.0 * gradient));

            float xOffset = sin((gradient * (Frequency * gradient)) + time) * Amplitude;
            coord.x += xOffset;

            vec4 colour = texture2D(u_texture, coord);
            colour.rgb *= u_lightness * gradient;

            gl_FragColor = colour;
        })";
}