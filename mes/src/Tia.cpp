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

#include "Tia.hpp"

#include <array>

namespace
{
    //these masks are used to make sure only the
    //valid bit(s) for this address is written
    //indexed by RegersIn enum
    std::array<std::uint8_t, 45> RegisterMasks =
    {
        0x02, 0xC2, 0x00, 0x00,
        0x37, 0x37,
        0xFE, 0xFE, 0xFE, 0xFE, 0x37,
        0x07, 0x07,
        0xF0, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x0F, 0x0F,
        0x1F, 0x1F,
        0x0F, 0x0F,
        0xFF, 0xFF,
        0x02, 0x02, 0x02,
        0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
        0x01, 0x01, 0x01,
        0x02, 0x02,
        0x00, 0x00,
        0x00
    };
}

Tia::Tia()
    : MappedDevice  (0, 0x7f),
    m_readyStatus   (ReadyStatus::High)
{
    //TODO pixel array for frame buffer

    //TODO create texture to display frame buffer

    //TODO check what the start up state of the registers should be
}

//public
std::uint8_t Tia::read(std::uint16_t addr, bool)
{
    XY_ASSERT(addr >= 0 && addr <= 0x0d, "Out of range for read!");
    return m_readRegisters[addr];
}

void Tia::write(std::uint16_t addr, std::uint8_t data)
{
    XY_ASSERT(addr >= 0 && addr <= 0x2c, "Out of range for write!");

    //callbacks for specific writes
    switch (addr)
    {
    default: 
        m_writeRegisters[addr] = data & RegisterMasks[addr];
        break;
    case CXCLR:
        //clear collisions
        cxclr();
        break;
    case HMCLR:
        //clear horizontal motion
        hmclr();
        break;
    case HMOVE:
        //apply horizontal motion
        hmove();
        break;
    case RESBL:
        //reset ball
        resbl();
        break;
    case RESM1:
        //reset missile 1
        resm1();
        break;
    case RESM0:
        //reset missile 0
        resm0();
        break;
    case RESP1:
        //reset player 1
        resp1();
        break;
    case RESP0:
        //reset player 0
        resp0();
        break;
    case RSync:
        //reset hor sync count
        rsync();
        break;
    case WSync:;
        //wait for hor blank
        wsync();
        break;
    }
}

void Tia::clock()
{
    //for each line tick the PF0-2 registers are serialised
    //as 20 bits on the left side of the line.
    //if CTRLPF 0 is true the bits are serialised in reverse
    //mirroring them across the right side of the line.

    //moveable objects are timed and are only included
    //in line sesrialisation when the timer is 0

    //timers are only clocked in the visible portion and
    //by default have same count as the visible area.
    //this way each line the object appears static. The
    //HMXX registers add or subtract time from the timers
    //causing the corresponding object to move left or right

    //the VDELxx registers decide whether the player or ball
    //drawables are serialised on this line

    //timers are only active when the ENAXX register enables
    //them (and thus output). The NUSIZX registers have 
    //specific bits dedicated to assigning certain widths
    //to each kind of drawable

    //TODO create an array to act as a serial line for each
    //object, including background. ANDing these values
    //together can be used to create the collision register
    //values, and lookup the final pixel output using the
    //pallet registers and priority registers.
}

//private
void Tia::wsync()
{
    //drives RDY pin low on 6502 - need to implement
    //RDY reset on next hblank signal
    m_readyStatus = ReadyStatus::Low;
}

void Tia::rsync()
{

}

void Tia::resp0()
{
    //reset p0 timer
}

void Tia::resp1() 
{
    //reset p1 timer
}

void Tia::resm0()
{
    //reset missile 0 timer
}

void Tia::resm1()
{
    //reset missile 1 timer
}

void Tia::resbl()
{
    //reset ball timer
}

void Tia::hmove()
{
    //copy the timing values from HMXX registers
    //into their relevant timers.
}

void Tia::hmclr()
{
    for (auto i = static_cast<int>(HMP0); i < VDELP0; ++i)
    {
        m_writeRegisters[i] = 0;
    }
}

void Tia::cxclr()
{
    for (auto i = 0; i < INPT0; ++i)
    {
        m_readRegisters[i] = 0;
    }
}