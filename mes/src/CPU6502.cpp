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

/*
Derived from the OneLoneCoder NES emulator tutorial
https://github.com/OneLoneCoder/olcNES

    Copyright 2018-2019 OneLoneCoder.com
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions or derivations of source code must retain the above
    copyright notice, this list of conditions and the following disclaimer.
    2. Redistributions or derivative works in binary form must reproduce
    the above copyright notice. This list of conditions and the following
    disclaimer must be reproduced in the documentation and/or other
    materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "CPU6502.hpp"
#include "MMU.hpp"

#include <sstream>
#include <iomanip>

CPU6502::CPU6502(MMU& mmu)
    : m_mmu     (mmu),
    m_fetched   (0),
    m_addrAbs   (0),
    m_addrRel   (0),
    m_opcode    (0),
    m_cycleCount(0),
    m_clockCount(0)
{
    //vector of instructions for the LUT. Copypasta from the olc version
    //to save my own sanity.
    using a = CPU6502;
    m_instructionTable =
    {
        { "BRK", &a::BRK, &a::IMM, 7 },{ "ORA", &a::ORA, &a::IZX, 6 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 3 },{ "ORA", &a::ORA, &a::ZP0, 3 },{ "ASL", &a::ASL, &a::ZP0, 5 },{ "???", &a::xxx, &a::IMP, 5 },{ "PHP", &a::PHP, &a::IMP, 3 },{ "ORA", &a::ORA, &a::IMM, 2 },{ "ASL", &a::ASL, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::NOP, &a::IMP, 4 },{ "ORA", &a::ORA, &a::ABS, 4 },{ "ASL", &a::ASL, &a::ABS, 6 },{ "???", &a::xxx, &a::IMP, 6 },
        { "BPL", &a::BPL, &a::REL, 2 },{ "ORA", &a::ORA, &a::IZY, 5 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "ORA", &a::ORA, &a::ZPX, 4 },{ "ASL", &a::ASL, &a::ZPX, 6 },{ "???", &a::xxx, &a::IMP, 6 },{ "CLC", &a::CLC, &a::IMP, 2 },{ "ORA", &a::ORA, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "ORA", &a::ORA, &a::ABX, 4 },{ "ASL", &a::ASL, &a::ABX, 7 },{ "???", &a::xxx, &a::IMP, 7 },
        { "JSR", &a::JSR, &a::ABS, 6 },{ "AND", &a::AND, &a::IZX, 6 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "BIT", &a::BIT, &a::ZP0, 3 },{ "AND", &a::AND, &a::ZP0, 3 },{ "ROL", &a::ROL, &a::ZP0, 5 },{ "???", &a::xxx, &a::IMP, 5 },{ "PLP", &a::PLP, &a::IMP, 4 },{ "AND", &a::AND, &a::IMM, 2 },{ "ROL", &a::ROL, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 2 },{ "BIT", &a::BIT, &a::ABS, 4 },{ "AND", &a::AND, &a::ABS, 4 },{ "ROL", &a::ROL, &a::ABS, 6 },{ "???", &a::xxx, &a::IMP, 6 },
        { "BMI", &a::BMI, &a::REL, 2 },{ "AND", &a::AND, &a::IZY, 5 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "AND", &a::AND, &a::ZPX, 4 },{ "ROL", &a::ROL, &a::ZPX, 6 },{ "???", &a::xxx, &a::IMP, 6 },{ "SEC", &a::SEC, &a::IMP, 2 },{ "AND", &a::AND, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "AND", &a::AND, &a::ABX, 4 },{ "ROL", &a::ROL, &a::ABX, 7 },{ "???", &a::xxx, &a::IMP, 7 },
        { "RTI", &a::RTI, &a::IMP, 6 },{ "EOR", &a::EOR, &a::IZX, 6 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 3 },{ "EOR", &a::EOR, &a::ZP0, 3 },{ "LSR", &a::LSR, &a::ZP0, 5 },{ "???", &a::xxx, &a::IMP, 5 },{ "PHA", &a::PHA, &a::IMP, 3 },{ "EOR", &a::EOR, &a::IMM, 2 },{ "LSR", &a::LSR, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 2 },{ "JMP", &a::JMP, &a::ABS, 3 },{ "EOR", &a::EOR, &a::ABS, 4 },{ "LSR", &a::LSR, &a::ABS, 6 },{ "???", &a::xxx, &a::IMP, 6 },
        { "BVC", &a::BVC, &a::REL, 2 },{ "EOR", &a::EOR, &a::IZY, 5 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "EOR", &a::EOR, &a::ZPX, 4 },{ "LSR", &a::LSR, &a::ZPX, 6 },{ "???", &a::xxx, &a::IMP, 6 },{ "CLI", &a::CLI, &a::IMP, 2 },{ "EOR", &a::EOR, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "EOR", &a::EOR, &a::ABX, 4 },{ "LSR", &a::LSR, &a::ABX, 7 },{ "???", &a::xxx, &a::IMP, 7 },
        { "RTS", &a::RTS, &a::IMP, 6 },{ "ADC", &a::ADC, &a::IZX, 6 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 3 },{ "ADC", &a::ADC, &a::ZP0, 3 },{ "ROR", &a::ROR, &a::ZP0, 5 },{ "???", &a::xxx, &a::IMP, 5 },{ "PLA", &a::PLA, &a::IMP, 4 },{ "ADC", &a::ADC, &a::IMM, 2 },{ "ROR", &a::ROR, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 2 },{ "JMP", &a::JMP, &a::IND, 5 },{ "ADC", &a::ADC, &a::ABS, 4 },{ "ROR", &a::ROR, &a::ABS, 6 },{ "???", &a::xxx, &a::IMP, 6 },
        { "BVS", &a::BVS, &a::REL, 2 },{ "ADC", &a::ADC, &a::IZY, 5 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "ADC", &a::ADC, &a::ZPX, 4 },{ "ROR", &a::ROR, &a::ZPX, 6 },{ "???", &a::xxx, &a::IMP, 6 },{ "SEI", &a::SEI, &a::IMP, 2 },{ "ADC", &a::ADC, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "ADC", &a::ADC, &a::ABX, 4 },{ "ROR", &a::ROR, &a::ABX, 7 },{ "???", &a::xxx, &a::IMP, 7 },
        { "???", &a::NOP, &a::IMP, 2 },{ "STA", &a::STA, &a::IZX, 6 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 6 },{ "STY", &a::STY, &a::ZP0, 3 },{ "STA", &a::STA, &a::ZP0, 3 },{ "STX", &a::STX, &a::ZP0, 3 },{ "???", &a::xxx, &a::IMP, 3 },{ "DEY", &a::DEY, &a::IMP, 2 },{ "???", &a::NOP, &a::IMP, 2 },{ "TXA", &a::TXA, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 2 },{ "STY", &a::STY, &a::ABS, 4 },{ "STA", &a::STA, &a::ABS, 4 },{ "STX", &a::STX, &a::ABS, 4 },{ "???", &a::xxx, &a::IMP, 4 },
        { "BCC", &a::BCC, &a::REL, 2 },{ "STA", &a::STA, &a::IZY, 6 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 6 },{ "STY", &a::STY, &a::ZPX, 4 },{ "STA", &a::STA, &a::ZPX, 4 },{ "STX", &a::STX, &a::ZPY, 4 },{ "???", &a::xxx, &a::IMP, 4 },{ "TYA", &a::TYA, &a::IMP, 2 },{ "STA", &a::STA, &a::ABY, 5 },{ "TXS", &a::TXS, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 5 },{ "???", &a::NOP, &a::IMP, 5 },{ "STA", &a::STA, &a::ABX, 5 },{ "???", &a::xxx, &a::IMP, 5 },{ "???", &a::xxx, &a::IMP, 5 },
        { "LDY", &a::LDY, &a::IMM, 2 },{ "LDA", &a::LDA, &a::IZX, 6 },{ "LDX", &a::LDX, &a::IMM, 2 },{ "???", &a::xxx, &a::IMP, 6 },{ "LDY", &a::LDY, &a::ZP0, 3 },{ "LDA", &a::LDA, &a::ZP0, 3 },{ "LDX", &a::LDX, &a::ZP0, 3 },{ "???", &a::xxx, &a::IMP, 3 },{ "TAY", &a::TAY, &a::IMP, 2 },{ "LDA", &a::LDA, &a::IMM, 2 },{ "TAX", &a::TAX, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 2 },{ "LDY", &a::LDY, &a::ABS, 4 },{ "LDA", &a::LDA, &a::ABS, 4 },{ "LDX", &a::LDX, &a::ABS, 4 },{ "???", &a::xxx, &a::IMP, 4 },
        { "BCS", &a::BCS, &a::REL, 2 },{ "LDA", &a::LDA, &a::IZY, 5 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 5 },{ "LDY", &a::LDY, &a::ZPX, 4 },{ "LDA", &a::LDA, &a::ZPX, 4 },{ "LDX", &a::LDX, &a::ZPY, 4 },{ "???", &a::xxx, &a::IMP, 4 },{ "CLV", &a::CLV, &a::IMP, 2 },{ "LDA", &a::LDA, &a::ABY, 4 },{ "TSX", &a::TSX, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 4 },{ "LDY", &a::LDY, &a::ABX, 4 },{ "LDA", &a::LDA, &a::ABX, 4 },{ "LDX", &a::LDX, &a::ABY, 4 },{ "???", &a::xxx, &a::IMP, 4 },
        { "CPY", &a::CPY, &a::IMM, 2 },{ "CMP", &a::CMP, &a::IZX, 6 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "CPY", &a::CPY, &a::ZP0, 3 },{ "CMP", &a::CMP, &a::ZP0, 3 },{ "DEC", &a::DEC, &a::ZP0, 5 },{ "???", &a::xxx, &a::IMP, 5 },{ "INY", &a::INY, &a::IMP, 2 },{ "CMP", &a::CMP, &a::IMM, 2 },{ "DEX", &a::DEX, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 2 },{ "CPY", &a::CPY, &a::ABS, 4 },{ "CMP", &a::CMP, &a::ABS, 4 },{ "DEC", &a::DEC, &a::ABS, 6 },{ "???", &a::xxx, &a::IMP, 6 },
        { "BNE", &a::BNE, &a::REL, 2 },{ "CMP", &a::CMP, &a::IZY, 5 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "CMP", &a::CMP, &a::ZPX, 4 },{ "DEC", &a::DEC, &a::ZPX, 6 },{ "???", &a::xxx, &a::IMP, 6 },{ "CLD", &a::CLD, &a::IMP, 2 },{ "CMP", &a::CMP, &a::ABY, 4 },{ "NOP", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "CMP", &a::CMP, &a::ABX, 4 },{ "DEC", &a::DEC, &a::ABX, 7 },{ "???", &a::xxx, &a::IMP, 7 },
        { "CPX", &a::CPX, &a::IMM, 2 },{ "SBC", &a::SBC, &a::IZX, 6 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "CPX", &a::CPX, &a::ZP0, 3 },{ "SBC", &a::SBC, &a::ZP0, 3 },{ "INC", &a::INC, &a::ZP0, 5 },{ "???", &a::xxx, &a::IMP, 5 },{ "INX", &a::INX, &a::IMP, 2 },{ "SBC", &a::SBC, &a::IMM, 2 },{ "NOP", &a::NOP, &a::IMP, 2 },{ "???", &a::SBC, &a::IMP, 2 },{ "CPX", &a::CPX, &a::ABS, 4 },{ "SBC", &a::SBC, &a::ABS, 4 },{ "INC", &a::INC, &a::ABS, 6 },{ "???", &a::xxx, &a::IMP, 6 },
        { "BEQ", &a::BEQ, &a::REL, 2 },{ "SBC", &a::SBC, &a::IZY, 5 },{ "???", &a::xxx, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "SBC", &a::SBC, &a::ZPX, 4 },{ "INC", &a::INC, &a::ZPX, 6 },{ "???", &a::xxx, &a::IMP, 6 },{ "SED", &a::SED, &a::IMP, 2 },{ "SBC", &a::SBC, &a::ABY, 4 },{ "NOP", &a::NOP, &a::IMP, 2 },{ "???", &a::xxx, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "SBC", &a::SBC, &a::ABX, 4 },{ "INC", &a::INC, &a::ABX, 7 },{ "???", &a::xxx, &a::IMP, 7 },
    };
}

//public
void CPU6502::reset()
{
    //the address 0xfffc contains the value of the address of the reset vector
    m_addrAbs = 0xfffc;
    std::uint16_t low = read(m_addrAbs);
    std::uint16_t high = read(m_addrAbs + 1);
    m_registers.pc = (high << 8) | low;

    m_registers.a = 0;
    m_registers.x = 0;
    m_registers.y = 0;
    m_registers.sp = 0xfd;
    m_registers.status = Flag::U;

    m_addrAbs = 0;
    m_addrRel = 0;
    m_fetched = 0;

    //reset takes 8 cycles
    m_cycleCount = 8;
}

void CPU6502::irq()
{
    if (getFlag(Flag::I) == 0)
    {
        doInterrupt(0xfffe);

        //set the cycle counter
        m_cycleCount = 7;
    }
}

void CPU6502::nmi()
{
    doInterrupt(0xfffa);
    m_cycleCount = 8;
}

void CPU6502::clock()
{
    if (m_cycleCount == 0)
    {
        m_opcode = read(m_registers.pc);

        setFlag(Flag::U, true);

        m_registers.pc++;

        m_cycleCount = m_instructionTable[m_opcode].cycles;

        //execute the address mode for this op - return value
        //indicates if we need to add any clock cycles
        auto ec1 = (this->*m_instructionTable[m_opcode].addrMode)();
        
        //perform the actual op, again returns if extra cycles are required
        auto ec2 = (this->*m_instructionTable[m_opcode].operate)();

        m_cycleCount += (ec1 & ec2);

        //operations may have mutated this
        setFlag(Flag::U, true);
    }

    m_clockCount++; //used for debug
    m_cycleCount--;
}

bool CPU6502::complete() const
{
    return m_cycleCount == 0;
}

std::map<std::uint16_t, std::string> CPU6502::dasm(std::uint16_t begin, std::uint16_t end) const
{
    std::uint32_t addr = begin;
    std::uint8_t value = 0;
    std::uint8_t high = 0;
    std::uint8_t low = 0;

    std::map<std::uint16_t, std::string> retVal;
    std::uint16_t currentLine = 0;

    auto hexStr = [](std::uint32_t val, std::uint8_t width)->std::string
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(width);
        ss << std::hex << val;
        return ss.str();
    };

    while (addr <= end)
    {
        currentLine = addr;

        //get the isntuction address and name
        std::string instruction("$" + hexStr(addr, 4) + ": ");
        std::uint8_t opcode = m_mmu.read(addr, true); //note we don't want to mutate state!
        addr++;
        instruction += m_instructionTable[opcode].name + " ";

        //use the address mode to complete the string...
        if (m_instructionTable[opcode].addrMode == &CPU6502::IMP)
        {
            instruction += " {IMP}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::IMM)
        {
            value = m_mmu.read(addr, true);
            addr++;
            instruction += "#$" + hexStr(value, 2) + " {IMM}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::ZP0)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = 0x00;
            instruction += "$" + hexStr(low, 2) + " {ZP0}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::ZPX)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = 0x00;
            instruction += "$" + hexStr(low, 2) + ", X {ZPX}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::ZPY)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = 0x00;
            instruction += "$" + hexStr(low, 2) + ", Y {ZPY}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::IZX)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = 0x00;
            instruction += "($" + hexStr(low, 2) + ", X) {IZX}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::IZY)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = 0x00;
            instruction += "($" + hexStr(low, 2) + "), Y {IZY}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::ABS)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = m_mmu.read(addr, true); 
            addr++;
            instruction += "$" + hexStr((uint16_t)(high << 8) | low, 4) + " {ABS}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::ABX)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = m_mmu.read(addr, true); 
            addr++;
            instruction += "$" + hexStr((uint16_t)(high << 8) | low, 4) + ", X {ABX}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::ABY)
        {
            low = m_mmu.read(addr, true); 
            addr++;
            high = m_mmu.read(addr, true); 
            addr++;
            instruction += "$" + hexStr((uint16_t)(high << 8) | low, 4) + ", Y {ABY}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::IND)
        {
            low = m_mmu.read(addr, true);
            addr++;
            high = m_mmu.read(addr, true); 
            addr++;
            instruction += "($" + hexStr((uint16_t)(high << 8) | low, 4) + ") {IND}";
        }
        else if (m_instructionTable[opcode].addrMode == &CPU6502::REL)
        {
            value = m_mmu.read(addr, true); 
            addr++;
            instruction += "$" + hexStr(value, 2) + " [$" + hexStr(addr + value, 4) + "] {REL}";
        }

        retVal[currentLine] = instruction;
    }

    return retVal;
}

//private
void CPU6502::doInterrupt(std::uint16_t vectorAddress)
{
    //push pc on the stack
    write(0x0100 + m_registers.sp, (m_registers.pc >> 8) & 0x00ff);
    m_registers.sp--;

    write(0x0100 + m_registers.sp, m_registers.pc & 0x00ff);
    m_registers.sp--;

    //update status register and push to stack
    setFlag(Flag::B, false);
    setFlag(Flag::U, true);
    setFlag(Flag::I, true);
    write(0x0100 + m_registers.sp, m_registers.status);
    m_registers.sp--;

    //grab the interrupt vector location from vector address
    m_addrAbs = vectorAddress;
    std::uint16_t low = read(m_addrAbs);
    std::uint16_t high = read(m_addrAbs + 1);
    m_registers.pc = (high << 8) | low;
}

std::uint8_t CPU6502::getFlag(Flag flag) const
{
    return ((m_registers.status & flag) > 0) ? 1 : 0;
}

void CPU6502::setFlag(Flag flag, bool b)
{
    if (b)
    {
        m_registers.status |= flag;
    }
    else
    {
        m_registers.status &= ~flag;
    }
}

std::uint8_t CPU6502::read(std::uint16_t address) const
{
    return m_mmu.read(address);
}

void CPU6502::write(std::uint16_t address, std::uint8_t data)
{
    m_mmu.write(address, data);
}

std::uint8_t CPU6502::fetch()
{
    if (!(m_instructionTable[m_opcode].addrMode == &CPU6502::IMP))
    {
        m_fetched = read(m_addrAbs);
    }

    return m_fetched;
}

//----------address modes-----------//


//-----------opcodes----------//