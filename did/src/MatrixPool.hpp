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
Pools matrices as a resource so they aren't invalidated
when components using them are moved in memory.
*/

#include "glm/mat4x4.hpp"
#include <xyginext/core/Assert.hpp>
#include <array>

class MatrixPool final
{
public:
    MatrixPool();

    std::size_t allocate();
    void deallocate(std::size_t);

    glm::mat4& at(std::size_t idx)
    {
        XY_ASSERT(idx < MaxMatrices, "Index out of range!");
        return m_matrices[idx];
    }

    const glm::mat4& at(std::size_t idx) const
    {
        XY_ASSERT(idx < MaxMatrices, "Index out of range!");
        return m_matrices[idx];
    }

private:
    static constexpr std::size_t MaxMatrices = 1024;
    std::array<glm::mat4, MaxMatrices> m_matrices = {};
    std::array<std::size_t, MaxMatrices> m_freeIndices;
    std::size_t m_nextFreeIndex;
};