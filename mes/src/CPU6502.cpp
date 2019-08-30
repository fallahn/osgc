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
#include "Util.hpp"

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
    m_addrAbs = ResetVector;
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
        doInterrupt(InterruptVector);

        //set the cycle counter
        m_cycleCount = 7;
    }
}

void CPU6502::nmi()
{
    doInterrupt(NMIVector);
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
    /*write(StackOffset + m_registers.sp, (m_registers.pc >> 8) & 0x00ff);
    m_registers.sp--;

    write(StackOffset + m_registers.sp, m_registers.pc & 0x00ff);
    m_registers.sp--;*/
    push((m_registers.pc >> 8) & 0x00ff);
    push(m_registers.pc & 0x00ff);

    //update status register and push to stack
    setFlag(Flag::B, false);
    setFlag(Flag::U, true);
    setFlag(Flag::I, true);
    /*write(StackOffset + m_registers.sp, m_registers.status);
    m_registers.sp--;*/
    push(m_registers.status);

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
std::uint8_t CPU6502::IMP()
{
    //implied mode
    m_fetched = m_registers.a;
    return 0;
}

std::uint8_t CPU6502::IMM()
{
    //immediate mode - next byte is value
    m_addrAbs = m_registers.pc++;
    return 0;
}

std::uint8_t CPU6502::ZP0()
{
    //zero page addressing
    m_addrAbs = read(m_registers.pc);
    m_registers.pc++;
    m_addrAbs &= 0x00ff;

    return 0;
}

std::uint8_t CPU6502::ZPX()
{
    //sero page with x offset - read address is offset by value of x reg
    m_addrAbs = read(m_registers.pc) + m_registers.x;
    m_registers.pc++;
    m_addrAbs &= 0x00ff;

    return 0;
}

std::uint8_t CPU6502::ZPY()
{
    //zero page + y offset
    m_addrAbs = read(m_registers.pc) + m_registers.y;
    m_registers.pc++;
    m_addrAbs &= 0x00ff;

    return 0;
}

std::uint8_t CPU6502::REL()
{
    //relative jump
    m_addrRel = read(m_registers.pc);
    m_registers.pc++;

    if (m_addrRel & 0x80)
    {
        m_addrRel |= 0xff00;
    }

    return 0;
}

std::uint8_t CPU6502::ABS()
{
    //absolute mode
    std::uint16_t low = read(m_registers.pc);
    m_registers.pc++;

    std::uint16_t high = read(m_registers.pc);
    m_registers.pc++;

    m_addrAbs = (high << 8) | low;

    return 0;
}

std::uint8_t CPU6502::ABX()
{
    //absolute address with offset from X register
    std::uint16_t low = read(m_registers.pc);
    m_registers.pc++;

    std::uint16_t high = (read(m_registers.pc) << 8);
    m_registers.pc++;

    m_addrAbs = high | low;
    m_addrAbs += m_registers.x;

    //if the offset moved the address into the next page
    //this mode requires another clock cycle
    if ((m_addrAbs & 0xff00) != high)
    {
        return 1;
    }

    return 0;
}

std::uint8_t CPU6502::ABY()
{
    //absolute address with offset from Y register
    std::uint16_t low = read(m_registers.pc);
    m_registers.pc++;

    std::uint16_t high = (read(m_registers.pc) << 8);
    m_registers.pc++;

    m_addrAbs = high | low;
    m_addrAbs += m_registers.y;

    //if the offset moved the address into the next page
    //this mode requires another clock cycle
    if ((m_addrAbs & 0xff00) != high)
    {
        return 1;
    }

    return 0;
}

std::uint8_t CPU6502::IND()
{
    //indirect address mode. The actual hardware has a bug
    //where crossing a page boundry wraps around to the beginning
    //of the same page...
    std::uint16_t low = read(m_registers.pc);
    m_registers.pc++;

    std::uint16_t high = (read(m_registers.pc) << 8);
    m_registers.pc++;

    std::uint16_t ptr = high | low;

    //this simulates the bug
    if (low == 0xff)
    {
        m_addrAbs = (read(ptr & 0xff00) << 8) | read(ptr);
    }
    else
    {
        m_addrAbs = (read(ptr + 1) << 8) | read(ptr);
    }

    return 0;
}

std::uint8_t CPU6502::IZX()
{
    //indirect mode with offset from x register.
    //the read value at pc has the x value added to it
    //to create a zero page address containing the
    //final value of the pointer

    std::uint16_t v = read(m_registers.pc);
    m_registers.pc++;

    std::uint16_t low = read(v + m_registers.x) & 0x00ff;
    std::uint16_t high = read(v + m_registers.x + 1) & 0x00ff;

    m_addrAbs = (high << 8) | low;

    return 0;
}

std::uint8_t CPU6502::IZY()
{
    //indirect with y offset. Unlike the
    //x offset the y register value is added
    //to the final pointer value. If this crosses
    //a page boundry and extra clock cycle is used

    std::uint16_t v = read(m_registers.pc);
    m_registers.pc++;

    //still zero paged however
    std::uint16_t low = read(v & 0x00ff);
    std::uint16_t high = (read((v + 1) & 0x00ff) << 8);

    m_addrAbs = high | low;
    m_addrAbs += m_registers.y;

    if ((m_addrAbs & 0xff00) != high)
    {
        return 1;
    }
    return 0;
}

//-----------opcodes----------//
std::uint8_t CPU6502::ADC()
{
    //add with carry
    fetch();

    //use a 16bit val just to catch the carry flag
    std::uint16_t temp = m_registers.a;
    temp += m_fetched;
    temp += getFlag(Flag::C);

    setFlag(Flag::Z, (temp & 0x00ff) == 0);

    if (getFlag(Flag::D))
    {
        //TODO a 65C02 would add an extra cycle - this is probably irrelevant here
        //m_cycleCount++;

        if (((m_registers.a & 0x0f) + (m_fetched & 0x0f) + getFlag(Flag::C)) > 9)
        {
            temp += 6;
        }

        setFlag(Flag::N, (temp & 0x80));

        //the overflow flag is based on the truth table
       //available at https://www.youtube.com/watch?v=8XmxKPJDGU0
        setFlag(Flag::V, (~(static_cast<std::uint16_t>(m_registers.a) ^ static_cast<std::uint16_t>(m_fetched))
            & (static_cast<std::uint16_t>(m_registers.a) ^ temp)) & 0x0080);

        if (temp > 0x99)
        {
            temp += 96;
        }
        setFlag(Flag::C, temp > 0x99);
    }
    else
    {
        setFlag(Flag::N, (temp & 0x80));
        setFlag(Flag::V, (~(static_cast<std::uint16_t>(m_registers.a) ^ static_cast<std::uint16_t>(m_fetched))
            & (static_cast<std::uint16_t>(m_registers.a) ^ temp)) & 0x0080);

        setFlag(Flag::C, (temp & 0xff00));
    }
    //put the result in the accumulator
    m_registers.a = (temp & 0x00ff);

    return 1; //this will require an extra cycle when used in certain address modes
}

std::uint8_t CPU6502::SBC()
{
    //subtract with borrow
    fetch();

    std::uint16_t v = m_fetched;
    v ^= 0x00ff;

    std::uint16_t temp = m_registers.a;
    temp += v;
    temp += getFlag(Flag::C);

    setFlag(Flag::Z, (temp & 0x00ff) == 0);
    setFlag(Flag::V, (temp ^ static_cast<std::uint16_t>(m_registers.a)) & (temp ^ v) & 0x0080);
    setFlag(Flag::N, temp & 0x80);

    if (getFlag(Flag::D))
    {
        //TODO a 65C02 would add an extra cycle - this is probably irrelevant here
        //m_cycleCount++;

        if (((m_registers.a & 0x0f) - getFlag(Flag::C)) < (v & 0x0f))
        {
            temp -= 6;
        }

        if (temp > 0x99)
        {
            temp -= 0x60;
        }
    }

    setFlag(Flag::C, (temp & 0xff00));

    m_registers.a = temp & 0x00ff;

    return 1;
}

std::uint8_t CPU6502::AND()
{
    //bitwise AND
    fetch();

    m_registers.a &= m_fetched;

    setFlag(Flag::Z, m_registers.a == 0);
    setFlag(Flag::N, (m_registers.a & 0x80));

    return 1;
}

std::uint8_t CPU6502::ASL()
{
    //shift left
    fetch();

    std::uint16_t temp = m_fetched;
    temp <<= 1;

    setFlag(Flag::C, (temp & 0xff00));
    setFlag(Flag::Z, (temp & 0x00ff) == 0);
    setFlag(Flag::N, (temp & 0x80));

    if (m_instructionTable[m_opcode].addrMode == &CPU6502::IMP)
    {
        //write back to accumulator
        m_registers.a = (temp & 0x00ff);
    }
    else
    {
        write(m_addrAbs, (temp & 0x00ff));
    }
    return 0;
}

std::uint8_t CPU6502::BCC() 
{
    //branch if carry cleared
    //if (getFlag(Flag::C) == 0)
    //{
    //    m_cycleCount++;
    //    m_addrAbs = m_registers.pc + m_addrRel;

    //    if ((m_addrAbs & 0xff00) != (m_registers.pc & 0xff00))
    //    {
    //        m_cycleCount++;
    //    }

    //    m_registers.pc = m_addrAbs;
    //}
    //return 0;
    return branch(getFlag(Flag::C) == 0);
}

std::uint8_t CPU6502::BCS()
{
    //branch if carry set
    //if (getFlag(Flag::C) != 0)
    //{
    //    m_cycleCount++;
    //    m_addrAbs = m_registers.pc + m_addrRel;

    //    if ((m_addrAbs & 0xff00) != (m_registers.pc & 0xff00))
    //    {
    //        m_cycleCount++;
    //    }

    //    m_registers.pc = m_addrAbs;
    //}
    //return 0;
    return branch(getFlag(Flag::C) != 0);
}

std::uint8_t CPU6502::BEQ()
{
    //branch if equal
    //if (getFlag(Flag::Z) == 1)
    //{
    //    m_cycleCount++;
    //    m_addrAbs = m_registers.pc + m_addrRel;

    //    if ((m_addrAbs & 0xff00) != (m_registers.pc & 0xff00))
    //    {
    //        m_cycleCount++;
    //    }

    //    m_registers.pc = m_addrAbs;
    //}
    //return 0;
    return branch(getFlag(Flag::Z) == 1);
}

std::uint8_t CPU6502::BIT()
{
    fetch();
    auto temp = m_registers.a & m_fetched;

    setFlag(Flag::Z, (temp & 0x00ff) == 0);
    setFlag(Flag::N, (m_fetched & 0x80));
    setFlag(Flag::V, (m_fetched & 0x40));

    return 0;
}

std::uint8_t CPU6502::BMI()
{
    //branch if negative
    return branch(getFlag(Flag::N) == 1);
}

std::uint8_t CPU6502::BNE()
{
    return branch(getFlag(Flag::Z) == 0);
}

std::uint8_t CPU6502::BPL()
{
    //branch of positive
    return branch(getFlag(Flag::N) == 0);
}

std::uint8_t CPU6502::BRK()
{
    //program break - store the program 
    //counter and status register on the stack
    m_registers.pc++;

    setFlag(Flag::I, true);
    /*write(StackOffset + m_registers.sp, (m_registers.pc >> 8) & 0x00ff);
    m_registers.sp--;
    write(StackOffset + m_registers.sp, (m_registers.pc & 0x00ff));
    m_registers.sp--;*/
    push((m_registers.pc >> 8) & 0x00ff);
    push((m_registers.pc & 0x00ff));

    setFlag(Flag::B, true);
    /*write(StackOffset + m_registers.sp, m_registers.status);
    m_registers.sp--;*/
    push(m_registers.status);
    setFlag(Flag::B, false);

    m_registers.pc = static_cast<std::uint16_t>(read(0xfffe)) | (static_cast<std::uint16_t>(read(0xffff) << 8));
    return 0;
}

std::uint8_t CPU6502::BVC()
{
    //branch if ovderflow clear
    return branch(getFlag(Flag::V) == 0);
}

std::uint8_t CPU6502::BVS()
{
    //branch if overflow set
    return branch(getFlag(Flag::V) == 1);
}

std::uint8_t CPU6502::CLC()
{
    //clear carry flag
    setFlag(Flag::C, false);
    return 0;
}

std::uint8_t CPU6502::CLD()
{
    //clear decimal flag
    setFlag(Flag::D, false);
    return 0;
}

std::uint8_t CPU6502::CLI()
{
    //clear interrupt flag (disables interrupts)
    setFlag(Flag::I, false);
    return 0;
}

std::uint8_t CPU6502::CLV()
{
    //clear overflow flag
    setFlag(Flag::V, false);
    return 0;
}

std::uint8_t CPU6502::CMP()
{
    //compare accumulator
    compare(m_registers.a);
    return 1;
}

std::uint8_t CPU6502::CPX()
{
    //compare x
    compare(m_registers.x);
    return 0;
}

std::uint8_t CPU6502::CPY()
{
    compare(m_registers.y);
    return 0;
}

std::uint8_t CPU6502::DEC()
{
    //decrements value at location
    fetch();
    std::uint16_t temp = m_fetched;
    temp -= 1;
    write(m_addrAbs, temp & 0x00ff);

    setFlag(Flag::Z, (temp & 0x00ff) == 0);
    setFlag(Flag::N, (temp & 0x80));

    return 0;
}

std::uint8_t CPU6502::DEX()
{
    //decrement x
    m_registers.x--;
    setFlag(Flag::Z, m_registers.x == 0);
    setFlag(Flag::N, (m_registers.x & 0x80));

    return 0;
}

std::uint8_t CPU6502::DEY()
{
    //decrement y
    m_registers.y--;
    setFlag(Flag::Z, m_registers.y == 0);
    setFlag(Flag::N, (m_registers.y & 0x80));

    return 0;
}

std::uint8_t CPU6502::EOR()
{
    //xor
    fetch();
    m_registers.a ^= m_fetched;
    setFlag(Flag::Z, m_registers.a == 0);
    setFlag(Flag::N, (m_registers.a & 0x80));

    return 1;
}

std::uint8_t CPU6502::INC()
{
    //increment at location
    fetch();
    std::uint16_t temp = m_fetched;
    temp += 1;

    write(m_addrAbs, (temp & 0x00ff));
    setFlag(Flag::Z, (temp & 0x00ff));
    setFlag(Flag::N, (temp & 0x80));

    return 0;
}

std::uint8_t CPU6502::INX()
{
    //increment x
    m_registers.x++;
    setFlag(Flag::Z, m_registers.x == 0);
    setFlag(Flag::N, m_registers.x & 0x80);

    return 0;
}

std::uint8_t CPU6502::INY()
{
    //increment y
    m_registers.y++;
    setFlag(Flag::Z, m_registers.y == 0);
    setFlag(Flag::N, m_registers.y & 0x80);

    return 0;
}

std::uint8_t CPU6502::JMP()
{
    m_registers.pc = m_addrAbs;
    return 0;
}

std::uint8_t CPU6502::JSR()
{
    m_registers.pc--;

    //push pc to stack
    /*write(StackOffset + m_registers.sp, (m_registers.pc >> 8) & 0x00ff);
    m_registers.sp--;
    write(StackOffset + m_registers.sp, (m_registers.pc & 0x00ff));
    m_registers.sp--;*/
    push((m_registers.pc >> 8) & 0x00ff);
    push((m_registers.pc & 0x00ff));

    m_registers.pc = m_addrAbs;

    return 0;
}

std::uint8_t CPU6502::LDA()
{
    return loadRegister(m_registers.a);
}

std::uint8_t CPU6502::LDX()
{
    return loadRegister(m_registers.x);
}

std::uint8_t CPU6502::LDY()
{
    return loadRegister(m_registers.y);
}

std::uint8_t CPU6502::LSR()
{
    //shift right
    fetch();
    setFlag(Flag::C, m_fetched & 0x01);

    std::uint16_t temp = m_fetched;
    temp >>= 1;

    setFlag(Flag::Z, (temp & 0x00ff) == 0);
    setFlag(Flag::N, (temp & 0x80));

    if (m_instructionTable[m_opcode].addrMode == &CPU6502::IMP)
    {
        m_registers.a = temp & 0x00ff;
    }
    else
    {
        write(m_addrAbs, temp & 0x00ff);
    }

    return 0;
}

std::uint8_t CPU6502::NOP()
{
    //not all NOPs behave the same.
    //see https://wiki.nesdev.com/w/index.php/CPU_unofficial_opcodes

    switch (m_opcode)
    {
    default: return 0;
        case 0x1c:
        case 0x3c:
        case 0x5c:
        case 0x7c:
        case 0xdc:
        case 0xfc:
            return 1;
    }
}

std::uint8_t CPU6502::ORA()
{
    //OR accumulator
    fetch();
    m_registers.a |= m_fetched;
    setFlag(Flag::Z, m_registers.a == 0);
    setFlag(Flag::N, m_registers.a & 0x80);

    return 1;
}

std::uint8_t CPU6502::PHA()
{
    //push accumulator to stack
    /*write(StackOffset + m_registers.sp, m_registers.a);
    m_registers.sp--;*/
    push(m_registers.a);

    return 0;
}

std::uint8_t CPU6502::PHP()
{
    //push status flags to stack
    /*write(StackOffset + m_registers.sp, m_registers.status | Flag::U | Flag::B);
    m_registers.sp--;*/
    push(m_registers.status | Flag::U | Flag::B);

    setFlag(Flag::U, false);
    setFlag(Flag::B, false);

    return 0;
}

std::uint8_t CPU6502::PLA()
{
    //pop accumulator
    /*m_registers.sp++;
    m_registers.a = read(StackOffset + m_registers.sp);*/
    m_registers.a = pop();

    setFlag(Flag::Z, m_registers.a == 0);
    setFlag(Flag::N, m_registers.a & 0x80);

    return 0;
}

std::uint8_t CPU6502::PLP()
{
    //pop status register
    /*m_registers.sp++;
    m_registers.status = read(StackOffset + m_registers.sp);*/
    m_registers.status = pop();
    setFlag(Flag::U, true);

    return 0;
}

std::uint8_t CPU6502::ROL()
{
    //rotate value left
    fetch();

    std::uint16_t temp = m_fetched;
    temp <<= 1;
    temp |= getFlag(Flag::C);

    setFlag(Flag::C, temp & 0xff00);
    setFlag(Flag::Z, (temp & 0x00ff) == 0 );
    setFlag(Flag::N, (temp & 0x80));

    if (m_instructionTable[m_opcode].addrMode == &CPU6502::IMP)
    {
        m_registers.a = temp & 0x00ff;
    }
    else
    {
        write(m_addrAbs, temp & 0x00ff);
    }

    return 0;
}

std::uint8_t CPU6502::ROR()
{
    //rotate right
    fetch();

    std::uint16_t temp = (getFlag(Flag::C) << 7);
    temp |= (m_fetched >> 1);

    setFlag(Flag::C, m_fetched & 0x01);
    setFlag(Flag::Z, (temp & 0x00ff) == 0);
    setFlag(Flag::N, temp & 0x80);

    if (m_instructionTable[m_opcode].addrMode == &CPU6502::IMP)
    {
        m_registers.a = temp & 0x00ff;
    }
    else
    {
        write(m_addrAbs, temp & 0x00ff);
    }

    return 0;
}

std::uint8_t CPU6502::RTI()
{
    //return from interrupt
    /*m_registers.sp++;
    m_registers.status = read(StackOffset + m_registers.sp);*/
    m_registers.status = pop();
    m_registers.status &= ~Flag::B;
    m_registers.status &= ~Flag::U;

    /*m_registers.sp++;
    m_registers.pc = read(StackOffset + m_registers.sp);
    m_registers.sp++;
    m_registers.pc |= static_cast<std::uint16_t>(read(StackOffset + m_registers.sp)) << 8;*/
    m_registers.pc = pop();
    m_registers.pc |= static_cast<std::uint16_t>(pop()) << 8;

    return 0;
}

std::uint8_t CPU6502::RTS()
{
    //return from subroutine
    /*m_registers.sp++;
    m_registers.pc = read(StackOffset + m_registers.sp);
    m_registers.sp++;
    m_registers.pc |= static_cast<std::uint16_t>(read(StackOffset + m_registers.sp)) << 8;*/
    m_registers.pc = pop();
    m_registers.pc |= static_cast<std::uint16_t>(pop()) << 8;

    m_registers.pc++;

    return 0;
}

std::uint8_t CPU6502::SEC()
{
    setFlag(Flag::C, true);
    return 0;
}

std::uint8_t CPU6502::SED()
{
    setFlag(Flag::D, true);
    return 0;
}

std::uint8_t CPU6502::SEI()
{
    setFlag(Flag::I, true);
    return 0;
}

std::uint8_t CPU6502::STA()
{
    //store accumulator
    write(m_addrAbs, m_registers.a);
    return 0;
}

std::uint8_t CPU6502::STX()
{
    //store x
    write(m_addrAbs, m_registers.x);
    return 0;
}

std::uint8_t CPU6502::STY()
{
    //store y
    write(m_addrAbs, m_registers.y);
    return 0;
}

std::uint8_t CPU6502::TAX()
{
    //transfer accumulator to x
    m_registers.x = m_registers.a;
    setFlag(Flag::Z, m_registers.x == 0);
    setFlag(Flag::N, m_registers.x & 0x80);

    return 0;
}

std::uint8_t CPU6502::TAY()
{
    //transfer accumulator to y
    m_registers.y = m_registers.a;
    setFlag(Flag::Z, m_registers.y == 0);
    setFlag(Flag::N, m_registers.y & 0x80);

    return 0;
}

std::uint8_t CPU6502::TSX()
{
    //transfer stack pointer to x
    m_registers.x = m_registers.sp;

    setFlag(Flag::Z, m_registers.x == 0);
    setFlag(Flag::N, m_registers.x & 0x80);

    return 0;
}

std::uint8_t CPU6502::TXA()
{
    //transfer x to accumulator
    m_registers.a = m_registers.x;

    setFlag(Flag::Z, m_registers.a == 0);
    setFlag(Flag::N, m_registers.a & 0x80);

    return 0;
}

std::uint8_t CPU6502::TXS()
{
    //transfer x to stack
    m_registers.sp = m_registers.x;
    return 0;
}

std::uint8_t CPU6502::TYA()
{
    m_registers.a = m_registers.y;

    setFlag(Flag::Z, m_registers.a == 0);
    setFlag(Flag::N, m_registers.a & 0x80);

    return 0;
}

std::uint8_t CPU6502::xxx()
{
    return 0;
}

//inlines
std::uint8_t CPU6502::branch(bool b)
{
    if (b)
    {
        m_cycleCount++;
        m_addrAbs = m_registers.pc + m_addrRel;

        if ((m_addrAbs & 0xff00) != (m_registers.pc & 0xff00))
        {
            m_cycleCount++;
        }

        m_registers.pc = m_addrAbs;
    }
    return 0;
}

void CPU6502::compare(std::uint8_t v)
{
    fetch();
    std::uint16_t temp = v;
    temp -= m_fetched;

    setFlag(Flag::C, temp >= m_fetched);
    setFlag(Flag::Z, (temp & 0x00ff) == 0);
    setFlag(Flag::N, (temp & 0x80));
}

std::uint8_t CPU6502::loadRegister(std::uint8_t& reg)
{
    fetch();
    reg = m_fetched;

    setFlag(Flag::Z, reg == 0);
    setFlag(Flag::N, reg & 0x80);

    return 1;
}

void CPU6502::push(std::uint8_t val)
{
    write(StackOffset + m_registers.sp, val);
    m_registers.sp++;
}

std::uint8_t CPU6502::pop()
{
    m_registers.sp--;
    return read(StackOffset + m_registers.sp);
}