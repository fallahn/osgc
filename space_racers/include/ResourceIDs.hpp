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
        Trail
    };
}

namespace FontID
{
    enum
    {
        Default,

        Count
    };

    static std::array<std::size_t, Count> handles = {};
}

namespace TextureID
{
    enum
    {
        MainMenu, VehicleSelect, Podium,
        Stars, StarsFar, StarsMid, StarsNear,
        RoidDiffuse, PlanetDiffuse, PlanetNormal,
        RoidShadow,

        VehicleNormal, VehicleSpecular, VehicleNeon,
        VehicleShadow, VehicleTrail,

        Temp01, Temp02,

        Count
    };

    //TODO tidy this up to limit texture IDs to correct scope(s)
    static std::array<std::size_t, Count> handles = {};
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