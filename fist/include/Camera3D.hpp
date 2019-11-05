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

//acts as a component containing 3D camera data

#include "glm/mat4x4.hpp"

#include <xyginext/ecs/System.hpp>


struct Camera3D final
{
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::mat4 viewProjectionMatrix;
    glm::mat4 rotationMatrix = glm::mat4(1.f);

    glm::mat4 postRotationMatrix = glm::mat4(1.f);
    glm::mat4 postTranslationMatrix = glm::mat4(1.f);

    static constexpr float defaultDepth = 4320.f;
    float depth = defaultDepth;

    float calcFOV(float viewYHeight, float targetDistance = defaultDepth)
    {
        //calcs the vertical FOV based on the camera distance from zero
        //(camera depth property, above), and the height of the rendered area
        return std::atan((viewYHeight / 2.f) / targetDistance);
    }
};

class Camera3DSystem final : public xy::System
{
public:
    explicit Camera3DSystem(xy::MessageBus& mb);

    void process(float dt) override;

private:

};