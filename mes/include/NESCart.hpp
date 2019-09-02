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

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

class NESCart final
{
public:
    NESCart();

    bool loadFromFile(const std::string&);

    const std::vector<std::uint8_t>& getROM() const { return m_PRGROM; }
    const std::vector<std::uint8_t>& getVROM() const { return m_CHRROM; }

    std::uint8_t getMapperID() const { return m_mapperID; }
    std::uint8_t getTableMirroring() const { return m_tableMirroring; }
    bool hasExtendedRAM() const { return m_hasExtendedRAM; }

    MappedDevice* getRomMapper();
    MappedDevice* getVRomMapper();

private:

    std::vector<std::uint8_t> m_PRGROM;
    std::vector<std::uint8_t> m_CHRROM;

    std::uint8_t m_mapperID;
    std::uint8_t m_tableMirroring;
    bool m_hasExtendedRAM;

    std::unique_ptr<MappedDevice> m_prgMapper;
    std::unique_ptr<MappedDevice> m_chrMapper;
};