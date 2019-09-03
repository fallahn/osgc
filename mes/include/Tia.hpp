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
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <array>

class Tia final : public MappedDevice, public sf::Drawable
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
    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite;
    ReadyStatus m_readyStatus;

    std::size_t m_currentLine;
    std::size_t m_currentClock;

    static constexpr std::size_t LinesPerFrame = 262;
    static constexpr std::size_t ClocksPerLine = 228;
    static constexpr std::size_t VSyncHeight = 3;
    static constexpr std::size_t VBlankHeight = 37 + VSyncHeight;
    static constexpr std::size_t OverscanHeight = LinesPerFrame - 30;
    static constexpr std::size_t HBlankWidth = 68;
    static constexpr std::size_t PictureWidth = 160;
    static constexpr std::size_t PictureHeight = 192;

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

    void draw(sf::RenderTarget&, sf::RenderStates) const;
};