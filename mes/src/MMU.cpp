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

#include "MMU.hpp"
#include "Util.hpp"

#include <xyginext/core/Assert.hpp>

MMU::MMU()
{
    for (auto& p : m_devices)
    {
        p = nullptr;
    }
}

//public
std::uint8_t MMU::read(std::uint16_t address, bool preventWrite)
{
    XY_ASSERT(address >= 0 && address <= 0xffff, "address out of range");
    return m_devices[address]->read(address, preventWrite);
}

void MMU::write(std::uint16_t address, std::uint8_t data)
{
    XY_ASSERT(address >= 0 && address <= 0xffff, "address out of range");
    m_devices[address]->write(address, data);
}

void MMU::mapDevice(MappedDevice& device)
{
    XY_ASSERT(device.rangeEnd() < m_devices.size(), "Device out of range");
    for (auto i = device.rangeStart(); i <= device.rangeEnd(); ++i)
    {
        if (m_devices[i] != nullptr)
        {
            xy::Logger::log("Overwrote mapped device " + m_devices[i]->name + " at $" + hexStr(i, 4) + " with " + device.name, xy::Logger::Type::Warning);
        }
        m_devices[i] = &device;
    }
}