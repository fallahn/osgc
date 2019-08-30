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
Mapped devices can be placed in memory space via the MMU
*/

#include <xyginext/core/Assert.hpp>

#include <cstdint>
#include <string>
#include <limits>
#include <vector>

class MappedDevice
{
public:
    MappedDevice(std::uint16_t start = 0, std::uint16_t end = std::numeric_limits<std::uint16_t>::max() - 1)
        : m_rangeStart(start), m_rangeEnd(end)
    {
        XY_ASSERT(end > start, "Invalid range");
    }

    virtual ~MappedDevice() = default;

    virtual std::uint8_t read(std::uint16_t, bool) = 0;
    virtual void write(std::uint16_t, std::uint8_t) = 0;

    std::string name; //for debugging

    std::uint16_t rangeStart() const { return m_rangeStart; }
    std::uint16_t rangeEnd() const { return m_rangeEnd; }

private:
    std::uint16_t m_rangeStart;
    std::uint16_t m_rangeEnd;
};

class RAMDevice : public MappedDevice
{
public:
    RAMDevice(std::uint16_t start, std::uint16_t end)
        : MappedDevice  (start, end),
        m_data          ((end - start) + 1)
    {
        name = "Generic RAM";
    }

    std::uint8_t read(std::uint16_t addr, bool preventWrite) override
    {
        XY_ASSERT(addr - rangeStart() < m_data.size(), "Out of range");
        return m_data[addr - rangeStart()];
    }

    void write(std::uint16_t addr, std::uint8_t data) override
    {
        XY_ASSERT(addr - rangeStart() < m_data.size(), "Out of range");
        m_data[addr - rangeStart()] = data;
    }

private:
    std::vector<std::uint8_t> m_data;
};