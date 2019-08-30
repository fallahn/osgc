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

#include "MappedDevice.hpp"

#include <vector>

/*
Memory mapping unit. Maps memory addresses to devices on a bus.

Examples:
    Atari 2600 (only has 13 bit bus)
    $0000 - $007F TIA
    $0080 - $00FF RAM

    $0280 - $02FF RIOT

    $1000 - $1FFF ROM

    NES
    $0000 - $07FF RAM
    $0800 - $0FFF RAM mirror
    $1000 - $17FF RAM mirror
    $1800 - $1FFF RAM mirror

    $2000 - $2007 PPU registers
    $2008 - $3FFF PPU mirrors

    $4000 - $4017 APU and IO registers
    $4018 - $401F extra IO (norm disabled)
    $4020 - $FFFF Cartridge space / mapper IO - note 6502 interrupt vector addresses are stored in the very end of this space
*/

class MMU final
{
public:
    MMU();

    //the boolean is used in the disassembly mode to prevent
    //mutating the state of attached devices when performing dasm
    std::uint8_t read(std::uint16_t address, bool preventWrite = false);
    void write(std::uint16_t address, std::uint8_t data);

    //maps a device to its defined range
    void mapDevice(MappedDevice&);

private:
    std::vector<MappedDevice*> m_devices;
};