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

#include "MappedDevice.hpp"

class MirroredRAM final : public MappedDevice
{
public:
    MirroredRAM(std::uint16_t start, std::uint16_t end, std::uint16_t count)
        : MappedDevice  (start, start + (((end - start) + 1) * count) - 1),
        m_size          ((end - start) + 1),
        m_count         (count)
    {
        name = "Mirrored RAM";

        m_ram.resize(static_cast<std::size_t>(m_size) * count);
    }

    std::uint8_t read(std::uint16_t addr, bool) override
    {
        XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Out of range");
        addr -= rangeStart();
        addr &= (m_size - 1);

        return m_ram[addr];
    }

    void write(std::uint16_t addr, std::uint8_t data) override
    {
        XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Out of range");
        addr -= rangeStart();
        addr &= (m_size - 1);

        for (auto i = 0; i < m_count; ++i)
        {
            m_ram[addr + (m_size * i)] = data;
        }
    }

private:
    std::uint16_t m_size;
    std::uint16_t m_count;

    std::vector<std::uint8_t> m_ram;
};