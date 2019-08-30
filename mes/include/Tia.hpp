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

#include <SFML/Graphics/Texture.hpp>

#include <array>

class Tia final : public MappedDevice
{
public:
    Tia();

    std::uint8_t read(std::uint16_t, bool) override;
    void write(std::uint16_t, std::uint8_t) override;

    void clock(); //perform one cycle

    const sf::Texture& getTexture() const { return m_texture; }

    enum RegistersIn
    {
        VSync, VBlank, WSync, RSync,
        NUSIZ0, NUSIZ1, 
        COLUP0, COLUP1, COLUPF, COLUBK, CTRLPF,
        REFP0, REFP1,
        PF0, PF1, PF2,
        RESP0, RESP1, RESM0, RESM1, RESBL,
        AUDC0, AUDC1,
        AUDF0, AUDF1,
        AUDV0, AUDV1,
        GRP0, GRP1,
        ENAM0, ENAM1, ENABL,
        HMP0, HMP1, HMM0, HMM1, HMBL,
        VDELP0, VDELP1, VDELBL,
        RESMP0, RESMP1,
        HMOVE, HMCLR,
        CXCLR
    };

    enum RegistersOut
    {
        CXM0P, CXM1P,
        CXP0FB, CXP1FB,
        CXM0FB, CXM1FB,
        CXBLPF, CXPPMM,
        INPT0, INPT1, INPT2,
        INPT3, INPT4, INPT5
    };

    enum class ReadyStatus
    {
        High, Low
    };
    ReadyStatus getRDYStatus() const
    {
        return m_readyStatus;
    }

private:
    sf::Texture m_texture;
    ReadyStatus m_readyStatus;

    std::array<std::uint8_t, 14u> m_readRegisters = {}; //contains collision data and controller input
    std::array<std::uint8_t, 45u> m_writeRegisters = {};

    void wsync();
    void rsync();
    void resp0();
    void resp1();
    void resm0();
    void resm1();
    void resbl();
    void hmove();
    void hmclr();
    void cxclr();
};