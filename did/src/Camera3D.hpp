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
    glm::vec3 worldPosition;
    float shakeAmount = 0.f;

    static constexpr float DefaultHeight = 40.f;
    float pitch = -0.06f; //radians
    float height = DefaultHeight;

    xy::Entity target;
};

class Camera3DSystem final : public xy::System
{
public:
    explicit Camera3DSystem(xy::MessageBus& mb);

    void process(float dt) override;

    void enableChase(bool enable);
    void setActive(bool active) { m_active = active; }

private:
    bool m_chaseEnabled;
    bool m_lockedOn;
    bool m_active;

    std::vector<float> m_shakeTable;
    std::size_t m_shakeIndex;
};