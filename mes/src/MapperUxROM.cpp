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
UxROMCPU::UxROMCPU(const NESCart& c)
    : MappedDevice      (0x8000, 0xffff),
    m_nesCart           (c),
    m_currentBank       (0),
    m_lastBankAddress   (0)
{
    m_lastBankAddress = c.getROM().size() - 0x4000;
    name = "UxROM CPU";
}

//public
std::uint8_t UxROMCPU::read(std::uint16_t addr, bool)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Address out of range");
    if (addr < 0xc000)
    {
        return m_nesCart.getROM()[(addr - 0x8000) + (m_currentBank << 14)];
    }

    return m_nesCart.getROM()[m_lastBankAddress + (addr - 0xc000)];
}

void UxROMCPU::write(std::uint16_t addr, std::uint8_t data)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Address out of range");
    m_currentBank = data & BankMask;
}

//------------------------------------------//
UxROMPPU::UxROMPPU(const NESCart& c)
    : MappedDevice  (0, 0x2000),
    m_nesCart       (c),
    m_hasRam        (false)
{
    if (c.getVROM().empty())
    {
        m_ram.resize(0x2000);
        m_hasRam;
    }

    name = "UxROM PPU";
}

//public
std::uint8_t UxROMPPU::read(std::uint16_t addr, bool)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Address out of range");
    if (m_hasRam)
    {
        return m_ram[addr];
    }
    return m_nesCart.getVROM()[addr];
}

void UxROMPPU::write(std::uint16_t addr, std::uint8_t data)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Address out of range");
    if (m_hasRam)
    {
        m_ram[addr] = data;
    }
}