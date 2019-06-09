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

//for complicated reasons this is used to create a matrix containing the
//inverse rotation of a drawable so it can be used in directional lighting
//shaders. See VehicleShader.inl for more info

#pragma once

#include "FastTrig.hpp"

#include <xyginext/ecs/System.hpp>
#include <xyginext/ecs/components/Transform.hpp>

//uses the fast trig sin/cos lookup table
//based on the source of sf::Transform
class Transform final
{
public:
    Transform()
    {
        m_matrix[0] = 1.f; m_matrix[4] = 0.f; m_matrix[8] = 0.f; m_matrix[12] = 0.f;
        m_matrix[1] = 0.f; m_matrix[5] = 1.f; m_matrix[9] = 0.f; m_matrix[13] = 0.f;
        m_matrix[2] = 0.f; m_matrix[6] = 0.f; m_matrix[10] = 1.f; m_matrix[14] = 0.f;
        m_matrix[3] = 0.f; m_matrix[7] = 0.f; m_matrix[11] = 0.f; m_matrix[15] = 1.f;
    }

    Transform(float a00, float a01, float a02,
              float a10, float a11, float a12,
              float a20, float a21, float a22)
    {
        m_matrix[0] = a00; m_matrix[4] = a01; m_matrix[8] = 0.f; m_matrix[12] = a02;
        m_matrix[1] = a10; m_matrix[5] = a11; m_matrix[9] = 0.f; m_matrix[13] = a12;
        m_matrix[2] = 0.f; m_matrix[6] = 0.f; m_matrix[10] = 1.f; m_matrix[14] = 0.f;
        m_matrix[3] = a20; m_matrix[7] = a21; m_matrix[11] = 0.f; m_matrix[15] = a22;
    }

    void setRotation(float angle)
    {
        float rad = angle * 3.141592654f / 180.f;
        float cos = ft::cos(rad);
        float sin = ft::sin(rad);

        *this = Transform(cos, -sin, 0.f,
                          sin, cos, 0.f,
                          0.f, 0.f, 1.f);
    }

    sf::Vector2f transformPoint(float x, float y)
    {
        return sf::Vector2f(m_matrix[0] * x + m_matrix[4] * y + m_matrix[12],
                            m_matrix[1] * x + m_matrix[5] * y + m_matrix[13]);
    }

    const float* getMatrix() const { return m_matrix.data(); }

private:
    std::array<float, 16u> m_matrix;
};

struct InverseRotation final
{
    Transform matrix;
};

class InverseRotationSystem final : public xy::System 
{
public:
    explicit InverseRotationSystem(xy::MessageBus& mb)
        : xy::System(mb, typeid(InverseRotationSystem))
    {
        requireComponent<xy::Transform>();
        requireComponent<InverseRotation>();
    }

    void process(float)
    {
        auto& entities = getEntities();
        for (auto entity : entities)
        {
            entity.getComponent<InverseRotation>().matrix.setRotation(-entity.getComponent<xy::Transform>().getRotation());
        }
    }
};