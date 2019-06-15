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
#include <array>
#include <string>

namespace ShaderID
{
    enum
    {
        Sprite3DTextured,
        Sprite3DColoured,
        TrackDistortion,
        Globe,
        Asteroid,
        Blur,
        NeonBlend,
        NeonExtract,
        Vehicle,
        Ghost,
        Trail,
        Stars,
        MonitorScreen,
        Lightbar
    };
}

namespace FontID
{
    enum
    {
        Default,

        Count
    };

    static const std::string DefaultFont("kingthings.ttf");
}

namespace TextureID
{
    namespace Menu
    {
        enum
        {
            MainMenu, VehicleSelect, Podium,
            Stars, StarsFar, StarsMid, StarsNear,
            PlanetDiffuse, PlanetNormal,
            MenuBackground, TrackSelect,
            LapFrame, LapCounter, LightBar,

            TimeTrialText, LocalPlayText,

            Count
        };
    }

    namespace Game
    {
        enum
        {
            Stars, StarsFar, StarsMid, StarsNear,
            RoidDiffuse, PlanetDiffuse, PlanetNormal,
            RoidShadow,
            VehicleNormal, VehicleSpecular, VehicleNeon,
            VehicleShadow, VehicleTrail,
            Fence, Chevron, Barrier, Pylon, Bollard,
            LapLine, LapProgress, LapPoint, LapCounter,

            NixieSheet,

            Count
        };
    }
}

namespace SpriteID::Menu
{
    enum
    {
        TimeTrialButton,
        LocalButton,
        NetButton,
        OptionsButton,
        QuitButton,

        PlayerNameButton,
        HostButton,
        AddressButton,
        JoinButton,
        NetBackButton,

        NavLeft, NavRight,
        Toggle,

        Count
    };
}

namespace SpriteID::Game
{
    enum
    {
        UIStartLights,
        Explosion,
        SmokePuff,

        Car,
        Bike,
        Ship,

        Count
    };
}