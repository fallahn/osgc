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

#include "Util.hpp"

#include <sstream>
#include <cmath>
#include <iomanip>

std::string formatTimeString(float t)
{
    float whole = 0.f;
    float remain = std::modf(t, &whole);

    int min = static_cast<int>(whole) / 60;
    int sec = static_cast<int>(whole) % 60;

    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << min << ":";
    ss << std::setw(2) << std::setfill('0') << sec << ":";
    ss << std::setw(4) << std::setfill('0') << static_cast<int>(remain * 10000.f);

    return ss.str();
}