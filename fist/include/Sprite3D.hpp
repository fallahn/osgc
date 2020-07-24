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
Responsible for creating the 3D matrix used for drawing sprites
*/

#include "MatrixPool.hpp"

#include <xyginext/ecs/System.hpp>

#include <vector>

struct Sprite3D final
{
public:
    Sprite3D()
        : depth             (0.f),
        m_pool              (nullptr),
        m_matrixIdx         (0){}

    explicit Sprite3D(MatrixPool& pool)
        : depth         (0.f),
        m_pool          (&pool),
        m_matrixIdx     (pool.allocate())
    {
    
    }

    ~Sprite3D()
    {
        if (m_pool)
        {
            m_pool->deallocate(m_matrixIdx);
        }
    }

    Sprite3D(Sprite3D&& other) noexcept
        : m_pool    (nullptr),
        m_matrixIdx (0)
    {
        std::swap(depth, other.depth);
        std::swap(m_pool, other.m_pool);
        std::swap(m_matrixIdx, other.m_matrixIdx);
    }

    Sprite3D& operator = (Sprite3D&& other) noexcept
    {
        std::swap(depth, other.depth);
        std::swap(m_pool, other.m_pool);
        std::swap(m_matrixIdx, other.m_matrixIdx);

        return *this;
    }

    glm::mat4& getMatrix()
    {
        XY_ASSERT(m_pool, "Created with missing construction parameter!");
        return m_pool->at(m_matrixIdx);
    }

    const glm::mat4& getMatrix() const
    {
        XY_ASSERT(m_pool, "Created with missing construction parameter!");
        return m_pool->at(m_matrixIdx);
    }

    float depth = 0.f;
    std::size_t renderPass = 0;
private:
    MatrixPool* m_pool;
    std::size_t m_matrixIdx;
};

class Sprite3DSystem final : public xy::System
{
public:
    Sprite3DSystem(xy::MessageBus&);

    void process(float) override;

private:

};