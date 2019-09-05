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

#include "ServerRoundTimer.hpp"

#include <algorithm>

namespace
{
    const std::int32_t Timeout = 3 * 60;
}

std::int32_t RoundTimer::getTime() const
{
    return std::max(0, Timeout - static_cast<std::int32_t>(m_clock.getElapsedTime().asSeconds()));
}

void RoundTimer::start()
{
    m_clock.restart();
    m_started = true;
}

bool RoundTimer::started() const
{
    return m_started;
}