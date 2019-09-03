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

CNROMCPU::CNROMCPU(NESCart& c)
    : MappedDevice  (0x8000, 0xffff),
    m_nesCart       (c),
    m_mirrored      (false)
{
    if (c.getROM().size() < 0x4000)
    {
        m_mirrored = true;
    }

    name = "CNROM CPU";
}

//public
std::uint8_t CNROMCPU::read(std::uint16_t addr, bool)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Out of range");
    if (m_mirrored && addr > 0xbfff)
    {
        return m_nesCart.getROM()[(addr - 0xc000)];
    }

    return m_nesCart.getROM()[addr - 0x8000];
}

void CNROMCPU::write(std::uint16_t addr, std::uint8_t data)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Out of range");
    static_cast<CNROMPPU*>(m_nesCart.getVRomMapper())->selectBank(data & BankMask);
}

//----------------------------------//
CNROMPPU::CNROMPPU(const NESCart& c)
    : MappedDevice(0, 0x2000),
    m_nesCart(c),
    m_currentBank(0)
{
    name = "CNROM PPU";
}

//public
std::uint8_t CNROMPPU::read(std::uint16_t addr, bool)
{
    XY_ASSERT(addr >= rangeStart() && addr <= rangeEnd(), "Out of range");
    return m_nesCart.getVROM()[addr + (m_currentBank << 13)];
}

void CNROMPPU::write(std::uint16_t, std::uint8_t)
{

}

void CNROMPPU::selectBank(std::uint8_t data)
{
    m_currentBank = data;
}