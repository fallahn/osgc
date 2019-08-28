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

#pragma once

#include <vector>
#include <string>
#include <map>

class MMU;

struct Registers final
{
    std::uint8_t a = 0;
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t sp = 0;
    std::uint16_t pc = 0;
    std::uint8_t status = 0;
};

/*
while this is mostly a complete emulation of the 6502 (suitable for the NES) 
the decimal flag is not implemented, as are 'undefined' opcodes. Using this
class for other emulation such as a C64 will need at least the Decimal mode
(D flag) implemented for ADC and SBC - as well as possibly the undefined 
opcodes which may be present in C64 software. See http://www.6502.org/tutorials/decimal_mode.html
*/

class CPU6502 final
{
public:
    enum Flag
    {
        C = 0x01,
        Z = 0x02,
        I = 0x04,
        D = 0x08, //note, currently unimplemented
        B = 0x10,
        U = 0x20,
        V = 0x40,
        N = 0x80
    };
    
    //these addresses should be programmed with the actual
    //address of the relevant vectors.
    static constexpr std::uint16_t NMIVector = 0xfffa;
    static constexpr std::uint16_t ResetVector = 0xfffc;
    static constexpr std::uint16_t InterruptVector = 0xfffe;

    CPU6502(MMU&);

    Registers& getRegisters() { return m_registers; }
    const Registers& getRegisters() const { return m_registers; }

    //calls reset vector
    void reset();

    //calls irq vector if interrupts enabled
    void irq();

    //non-maskable interrupt vector
    void nmi();

    //cycle CPU
    void clock();

    //returns true if current instruction complete
    bool complete() const;

    //disassembler
    std::map<std::uint16_t, std::string> dasm(std::uint16_t begin, std::uint16_t end) const;

private:

    MMU& m_mmu;
    Registers m_registers;

    std::uint8_t m_fetched; //current working value
    std::uint16_t m_addrAbs; //absolute working address
    std::uint16_t m_addrRel; //absolute address after a relative jump
    std::uint8_t m_opcode; //current opcode
    std::uint8_t m_cycleCount; //current cycle count of active opcode (counts down to 0)
    std::uint32_t m_clockCount; //total number of emulation cycles

    struct Instruction final
    {
        std::string name; //used in dasm
        std::uint8_t(CPU6502::* operate)() = nullptr;
        std::uint8_t(CPU6502::* addrMode)() = nullptr;
        std::uint8_t cycles = 0;
    };
    std::vector<Instruction> m_instructionTable;

    void doInterrupt(std::uint16_t vectorLocation);

    std::uint8_t getFlag(Flag) const;
    void setFlag(Flag, bool);

    //read/write MMU
    std::uint8_t read(std::uint16_t addr) const;
    void write(std::uint16_t addr, std::uint8_t data);

    //may read from MMU, may fetch opcode data, depending on opcode
    std::uint8_t fetch();

    //unlike a Z80 the 6502 doesn't have distinct opcodes for every
    //possibility, rather it has the ability to change addressing
    //mode which affects how the current opcode performs.
    std::uint8_t IMP();
    std::uint8_t IMM();
    std::uint8_t ZP0();
    std::uint8_t ZPX();
    std::uint8_t ZPY();
    std::uint8_t REL();
    std::uint8_t ABS();
    std::uint8_t ABX();
    std::uint8_t ABY();
    std::uint8_t IND();
    std::uint8_t IZX();
    std::uint8_t IZY();

    //the 56 'defined' opcodes which strictly speaking makes this
    //6502 compliant. Undocumented opcodes are not implemented
    //(the emulation doesn't take a microcode approach) so any
    //software designed to take advantage of these will most likely
    //not work. Opcodes which require an additional cycle to complete
    //due to the current addressing mode will return 1, otherwise they return 0
    std::uint8_t ADC();	std::uint8_t AND();	std::uint8_t ASL();	std::uint8_t BCC();
    std::uint8_t BCS();	std::uint8_t BEQ();	std::uint8_t BIT();	std::uint8_t BMI();
    std::uint8_t BNE();	std::uint8_t BPL();	std::uint8_t BRK();	std::uint8_t BVC();
    std::uint8_t BVS();	std::uint8_t CLC();	std::uint8_t CLD();	std::uint8_t CLI();
    std::uint8_t CLV();	std::uint8_t CMP();	std::uint8_t CPX();	std::uint8_t CPY();
    std::uint8_t DEC();	std::uint8_t DEX();	std::uint8_t DEY();	std::uint8_t EOR();
    std::uint8_t INC();	std::uint8_t INX();	std::uint8_t INY();	std::uint8_t JMP();
    std::uint8_t JSR();	std::uint8_t LDA();	std::uint8_t LDX();	std::uint8_t LDY();
    std::uint8_t LSR();	std::uint8_t NOP();	std::uint8_t ORA();	std::uint8_t PHA();
    std::uint8_t PHP();	std::uint8_t PLA();	std::uint8_t PLP();	std::uint8_t ROL();
    std::uint8_t ROR();	std::uint8_t RTI();	std::uint8_t RTS();	std::uint8_t SBC();
    std::uint8_t SEC();	std::uint8_t SED();	std::uint8_t SEI();	std::uint8_t STA();
    std::uint8_t STX();	std::uint8_t STY();	std::uint8_t TAX();	std::uint8_t TAY();
    std::uint8_t TSX();	std::uint8_t TXA();	std::uint8_t TXS();	std::uint8_t TYA();

    //used to catch unimplemented/unofficial ops
    std::uint8_t xxx();

    //util funcs for functionally similar ops
    inline std::uint8_t branch(bool);
    inline void compare(std::uint8_t);
    inline std::uint8_t loadRegister(std::uint8_t&);
};