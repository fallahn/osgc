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

//iNES format http://nesdev.com/neshdr20.txt

#include "NESCart.hpp"
#include "Mapper.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/core/Assert.hpp>

#include <fstream>

NESCart::NESCart()
    : m_mapperID    (0),
    m_tableMirroring(0),
    m_hasExtendedRAM(false)
{

}

//public
bool NESCart::loadFromFile(const std::string& path)
{
    m_mapperID = 0;
    m_tableMirroring = 0;
    m_hasExtendedRAM = 0;
    m_CHRROM.clear();
    m_PRGROM.clear();

    m_chrMapper.reset();
    m_prgMapper.reset();

    std::ifstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        std::vector<std::uint8_t> header(0x10);

        if (!file.read(reinterpret_cast<char*>(header.data()), 0x10))
        {
            xy::Logger::log("Failed reading iNES header", xy::Logger::Type::Error);
            return false;
        }

        //header ident
        std::string ident(header.data(), header.data() + 4);
        if (ident != "NES\x1A")
        {
            xy::Logger::log("NES ident was " + ident + " - this is not correct", xy::Logger::Type::Error);
            return false;
        }

        //PRG bank count
        std::uint8_t bankCount = header[4];
        if (bankCount == 0)
        {
            xy::Logger::log("Invalid PRG bank count, must be at least 1!", xy::Logger::Type::Error);
            return false;
        }

        //CHR bank count - this may be 0
        std::uint8_t vbankCount = header[5];

        //flags are in byte 6. TODO fully parse all flag info
        m_tableMirroring = header[6] & 0x0b;

        m_mapperID = ((header[6] >> 4) & 0x0f) | (header[7] & 0xf0);

        m_hasExtendedRAM = (header[6] & 0x02) != 0;

        //some unimplemented stuff
        if (header[6] & 0x04)
        {
            xy::Logger::log("Trainer is not implemented!", xy::Logger::Type::Warning);
        }

        if ((header[0x0a] & 0x03) == 0x02 || (header[0x0a] & 0x01))
        {
            xy::Logger::log("PAL ROMs not yet implemented... sorry!", xy::Logger::Type::Error);
            return false;
        }

        //load ROM data
        m_PRGROM.resize(bankCount * 0x4000);
        if (!file.read(reinterpret_cast<char*>(m_PRGROM.data()), m_PRGROM.size()))
        {
            xy::Logger::log("Failed reading PRG ROM", xy::Logger::Type::Error);
            return false;
        }

        if (vbankCount)
        {
            m_CHRROM.resize(vbankCount * 0x2000);
            if (!file.read(reinterpret_cast<char*>(m_CHRROM.data()), m_CHRROM.size()))
            {
                xy::Logger::log("Failed reading CHR ROM", xy::Logger::Type::Error);
                return false;
            }
            xy::Logger::log("Read " + std::to_string(m_CHRROM.size()) + " bytes of CHR ROM", xy::Logger::Type::Info);
        }

        //attempt to create ROM mapper
        switch (m_mapperID)
        {
        default:
            xy::Logger::log("Mapper " + std::to_string(m_mapperID) + " not implemented!", xy::Logger::Type::Error);
            return false;
        case Mapper::NROM:
            LOG("Created NROM mapper", xy::Logger::Type::Info);
            m_prgMapper = std::make_unique<Mapper::NROMCPU>(*this);
            m_chrMapper = std::make_unique<Mapper::NROMPPU>(*this);
            break;
        case Mapper::UxROM:
            LOG("Created UxROM mapper", xy::Logger::Type::Info);
            m_prgMapper = std::make_unique<Mapper::UxROMCPU>(*this);
            m_chrMapper = std::make_unique<Mapper::UxROMPPU>(*this);
            break;
        /*case Mapper::CNROM:
            LOG("Created CNROM mapper", xy::Logger::Type::Info);
            m_prgMapper = std::make_unique<Mapper::CNROMCPU>(*this);
            m_chrMapper = std::make_unique<Mapper::CNROMPPU>(*this);
            break;*/
        }

        return true;
    }
    xy::Logger::log("Failed to open " + path, xy::Logger::Type::Error);

    return false;
}

MappedDevice* NESCart::getRomMapper()
{
    XY_ASSERT(m_prgMapper, "PRG mapper has not been created!");
    return m_prgMapper.get();
}

MappedDevice* NESCart::getVRomMapper()
{
    XY_ASSERT(m_chrMapper, "CHR mapper has not been created!");
    return m_chrMapper.get();
}