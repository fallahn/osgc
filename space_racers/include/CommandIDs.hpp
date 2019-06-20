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

namespace CommandID
{
    enum Game
    {
        NetActor = 0x1,
        Trail = 0x2,
        Vehicle = 0x4,
        LapLine = 0x8
    };

    enum UI
    {
        StartLights = 0x1,
        LapText = 0x2,

        TimeText = 0x8,
        BestTimeText = 0x10,
        TopTimesText = 0x20,
        TopTimesBoard = 0x40,

        EliminationDot = 0x80
    };

    enum Lobby
    {
        PlayerText = 0x1,
        LobbyText = 0x2,
    };


    enum Menu
    {
        RootNode = 0x1,
        NameText = 0x2,
        IPText = 0x4,
        TrackThumb = 0x8,
        LapCounter = 0x10
    };
}
