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

class NESCart;
namespace Mapper
{
    enum Type
    {
        //generic ROM. $8000 - $bfff First 16kb, $c000 - $ffff Second 16kb, or mirror of first
        //CHR rom 8Kb, nto bank switched. Optionally CHR ram for some homebrew, mapped from 
        //$0000 on the PPU bus. $6000-$7ffff on the CPU bus read/write for famicom basic
        NROM = 0, 
        //SxROM. $6000-$7fff optional RAM. $8000-$bfff switchable low bank. $c000 - $ffff switchable
        //high bank. PPU banks $0000 - $0fff and $1000 - $1fff
        SxROM = 1,
        //UxROM - $8000 - $bfff is switchable. $c000 - $ffff is fixed to last bank.
        //note only bits 3 - 0 are used in bank select register. Some variants have fixed CHR
        //ROM
        UxROM = 2,
        //CNROM - ROM is fixed at either $8000 - $bfff or $8000 - $ffff.
        //writing bank select register ($8000 - $ffff) selects the CHR bank mapped to $0000-$1ffff PPU
        CNROM = 3,

        Count //if a cartridge returns >= this then mapper is not implemented
    };

    enum Mirroring
    {
        Horizontal = 0,
        Vertical = 1,
        FourScreen = 8,
        OneScreenLower,
        OneScreenHigher
    };

    //mapped to CPU bus
    class NROMCPU final : public MappedDevice
    {
    public:
        explicit NROMCPU(const NESCart&);

        std::uint8_t read(std::uint16_t address, bool noMutate) override;
        void write(std::uint16_t address, std::uint8_t data) override;

    private:
        const NESCart& m_nesCart;
        bool m_multipage;

        std::vector<std::uint8_t> m_basicRAM;
    };

    //mapped to PPU
    class NROMPPU final : public MappedDevice
    {
    public:
        explicit NROMPPU(const NESCart&);

        std::uint8_t read(std::uint16_t address, bool noMutate) override;
        void write(std::uint16_t address, std::uint8_t data) override;

    private:
        const NESCart& m_nesCart;

        std::vector<std::uint8_t> m_charRAM; //used if CHRROM is empty/not used
    };

    //-------------------------------------//
    class UxROMCPU final : public MappedDevice
    {
    public:
        explicit UxROMCPU(const NESCart&);
        
        std::uint8_t read(std::uint16_t address, bool noMutate) override;
        void write(std::uint16_t address, std::uint8_t data) override;

    private:
        const NESCart& m_nesCart;
        std::uint8_t m_currentBank;
        std::size_t m_lastBankAddress;
        static constexpr std::uint8_t BankMask = 0x03;
    };

    //UxROM has CHR RAM in special cases
    class UxROMPPU final : public MappedDevice
    {
    public:
        explicit UxROMPPU(const NESCart&);

        std::uint8_t read(std::uint16_t, bool) override;
        void write(std::uint16_t, std::uint8_t) override;

    private:
        const NESCart& m_nesCart;
        bool m_hasRam;
        std::vector<std::uint8_t> m_ram;
    };

    //-------------------------------------//
    class CNROMCPU final : public MappedDevice
    {
        explicit CNROMCPU(NESCart&); //needs to be able to write to the CHR mapper

        std::uint8_t read(std::uint16_t address, bool noMutate) override;
        void write(std::uint16_t address, std::uint8_t data) override;

    private:
        NESCart& m_nesCart;
        bool m_mirrored;
        static constexpr std::uint8_t BankMask = 0x03;
    };

    class CNROMPPU final : public MappedDevice
    {
    public:
        explicit CNROMPPU(const NESCart&);

        std::uint8_t read(std::uint16_t address, bool noMutate) override;
        void write(std::uint16_t address, std::uint8_t data) override;

        void selectBank(std::uint8_t);

    private:
        const NESCart& m_nesCart;
        std::uint8_t m_currentBank;
    };
}
