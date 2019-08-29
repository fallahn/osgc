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

#pragma once

#include "MMU.hpp"
#include "CPU6502.hpp"
#include "MappedDevice.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/gui/GuiClient.hpp>

class MainState final : public xy::State, public xy::GuiClient
{
public:
    MainState(xy::StateStack&, xy::State::Context);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:

    MMU m_mmu;
    CPU6502 m_cpu;
    RAMDevice m_ram;

    std::map<std::uint16_t, std::string> m_dasm;
};