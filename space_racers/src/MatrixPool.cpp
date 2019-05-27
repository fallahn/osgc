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

#include "MatrixPool.hpp"

MatrixPool::MatrixPool()
    : m_nextFreeIndex(MaxMatrices)
{
    for (auto i = 0u; i < MaxMatrices; ++i)
    {
        m_freeIndices[i] = i;
    }
}

//public
std::size_t MatrixPool::allocate()
{
    XY_ASSERT(m_nextFreeIndex > 0, "Max matrices have been allocated!");
    return m_freeIndices[--m_nextFreeIndex];
}

void MatrixPool::deallocate(std::size_t idx)
{
    XY_ASSERT(idx < MaxMatrices, "Index out of range!");
    XY_ASSERT(m_nextFreeIndex < MaxMatrices, "Everything is deallocated, something went wrong!");
    m_freeIndices[m_nextFreeIndex++] = idx;
}