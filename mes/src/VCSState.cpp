/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "VCSState.hpp"
#include "StateIDs.hpp"
#include "Util.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/gui/Gui.hpp>

#include <SFML/Window/Event.hpp>

namespace
{
    std::array<std::uint8_t, 32> testPrg =
    {
        0xA2, 0x0A,       //ldx 10
        0x8E, 0x80, 0x00, //stx 128
        0xA2, 0x03,       //ldx 3
        0x8E, 0x81, 0x00, //stx 129
        0xAC, 0x80, 0x00, //ldy fromr $80
        0xA9, 0x00,       //lda 0
        0x18,             //clc
                          //loop
        0x6D, 0x81, 0x00, //adc from $81
        0x88,             //dey
        0xD0, 0xFA,       //bne loop
        0x8D, 0x82, 0x00, //sta 130
        0xEA,             //nop
        0xEA,             //nop
        0xEA,             //nop
        0xEA,             //nop
        0x4C, 0x00, 0x10  //jmp 0x1000
    };
}

VCSState::VCSState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State (ss, ctx),
    m_cpu       (m_mmu),
    m_ram       (0x80, 0xFF),
    m_vectorRAM (CPU6502::ResetVector, 0xffff),
    m_tempROM   (0x1000, 0x1fff)
{
    m_vectorRAM.name = "Vector RAM";

    //map devices
    m_mmu.mapDevice(m_tia);
    m_mmu.mapDevice(m_ram);
    m_mmu.mapDevice(m_tempROM);
    m_mmu.mapDevice(m_vectorRAM);    

    //load the test program
    std::uint16_t prgOffset = 0x1000;
    for (auto b : testPrg)
    {
        m_mmu.write(prgOffset++, b);
    }

    //set the reset vector
    m_mmu.write(CPU6502::ResetVector, 0x00);
    m_mmu.write(CPU6502::ResetVector + 1, 0x10);

    m_dasm = m_cpu.dasm(0x1000, 0x1000 + testPrg.size() - 1);

    m_cpu.reset();

    //set up some UI stuff for printing info
    namespace ui = xy::ui;
    registerWindow([&]() 
        {
            ui::begin("Disassembly");

            const auto& registers = m_cpu.getRegisters();
            ui::text("PC: $" + hexStr(registers.pc, 4));
            ui::text("SP: $" + hexStr(registers.sp, 2));
            ui::text(" A: $" + hexStr(registers.a, 2));
            ui::text(" X: $" + hexStr(registers.x, 2));
            ui::text(" Y: $" + hexStr(registers.y, 2));
            ui::text("Flags: C Z I D B U V N");
            ui::text("       "
                + std::to_string((registers.status & CPU6502::C) ? 1 : 0) + " "
                + std::to_string((registers.status & CPU6502::Z) ? 1 : 0) + " "
                + std::to_string((registers.status & CPU6502::I) ? 1 : 0) + " "
                + std::to_string((registers.status & CPU6502::D) ? 1 : 0) + " "
                + std::to_string((registers.status & CPU6502::B) ? 1 : 0) + " "
                + std::to_string((registers.status & CPU6502::U) ? 1 : 0) + " "
                + std::to_string((registers.status & CPU6502::V) ? 1 : 0) + " "
                + std::to_string((registers.status & CPU6502::N) ? 1 : 0));

            ui::separator();

            for (const auto& [addr, val] : m_dasm)
            {
                
                if (addr == registers.pc)
                {
                    
                    ui::text("PC");
                }
                else
                {
                    ui::text("  ");
                }

                ui::sameLine();
                ui::text(val);
            }

            ui::end();
        });

    registerWindow([&]() 
        {
            ui::begin("RAM view");
            static std::uint16_t start = 128;
            static std::uint16_t end = 192;

            static const std::uint16_t ColWidth = 8;
            for (auto i = start; i < end;)
            {
                std::string str;
                for (auto j = 0; j < ColWidth; ++j)
                {
                    str += hexStr(m_mmu.read(i++), 2) + " ";
                }
                ui::text(str);
            }

            ui::end();
        });
}

//public
bool VCSState::handleEvent(const sf::Event& evt)
{
	//prevents events being forwarded if the console wishes to consume them
	/*if (xy::ui::wantsKeyboard() || xy::ui::wantsMouse())
    {
        return true;
    }*/

    if (evt.type == sf::Event::KeyPressed)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::Space:
            do
            {
                m_cpu.clock();

                m_tia.clock();
                m_tia.clock();
                m_tia.clock();

            } while (!m_cpu.complete());
            break;
        case sf::Keyboard::Escape:
            xy::App::quit();
            break;
        }
    }

    return true;
}

void VCSState::handleMessage(const xy::Message&)
{

}

bool VCSState::update(float)
{
    return true;
}

void VCSState::draw()
{

}

xy::StateID VCSState::stateID() const
{
    return StateID::VCS2600;
}