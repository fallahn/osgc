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

#include "Mapper.hpp"
#include "NESCart.hpp"

using namespace Mapper;

NROMCPU::NROMCPU(const NESCart& c)
    : MappedDevice(0x6000, 0xffff),
    m_nesCart   (c),
    m_multipage (c.getROM().size() > 0x4000),
    m_basicRAM  (0x2000)
{
    name = "NROM CPU";
}

//public
std::uint8_t NROMCPU::read(std::uint16_t addr, bool)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Address out of range");
    
    if (addr < 0x8000)
    {
        return m_basicRAM[addr];
    }
    
    if (m_multipage)
    {
        return m_nesCart.getROM()[addr - 0x8000];
    }

    return m_nesCart.getROM()[(addr - 0x8000) & 0x3fff];
}

void NROMCPU::write(std::uint16_t addr, std::uint8_t value)
{
    //XY_ASSERT(addr >= rangeStart() && addr < rangeStart() + m_basicRAM.size(), "Address out of range");
    if (addr < 0x8000)
    {
        m_basicRAM[addr - 0x6000] = value;
    }
    //LOG("Attempt to write to ROM at " + std::to_string(addr), xy::Logger::Type::Warning);
}

//------------------------//
NROMPPU::NROMPPU(const NESCart& c)
    : MappedDevice(0, 0x2000),
    m_nesCart(c)
{
    if (c.getVROM().empty())
    {
        m_charRAM.resize(0x2000);
    }

    name = "NROM PPU";
}

std::uint8_t NROMPPU::read(std::uint16_t addr, bool)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Address out of range");
    if (m_charRAM.empty())
    {
        return m_nesCart.getVROM()[addr];
    }

    return m_charRAM[addr];
}

void NROMPPU::write(std::uint16_t addr, std::uint8_t value)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Address out of range");
    if (!m_charRAM.empty()) 
    {
        m_charRAM[addr] = value;
    }
}