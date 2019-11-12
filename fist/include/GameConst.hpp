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

#pragma once

#include <cstdint>

namespace GameConst
{
    static const float RoomWidth = 960.f;
    static const float RoomHeight = 540.f;
    static const float RoomPadding = 0.f;// 12.f;
    static const float WallThickness = 12.f;
    static const std::int32_t RoomsPerRow = 8;

    static const float CamTranslateSpeed = 10.f;
    static const float CamRotateSpeed = 20.f;
}
