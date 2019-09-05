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

#include <array>
#include <string>

namespace Server
{
    namespace Message
    {
        enum
        {
            OneTreasureRemaining,
            PlayerOneDrowned,
            PlayerTwoDrowned,
            PlayerThreeDrowned,
            PlayerFourDrowned,
            PlayerOneDiedFromWeapon,
            PlayerTwoDiedFromWeapon,
            PlayerThreeDiedFromWeapon,
            PlayerFourDiedFromWeapon,
            PlayerOneDiedFromSkeleton,
            PlayerTwoDiedFromSkeleton,
            PlayerThreeDiedFromSkeleton,
            PlayerFourDiedFromSkeleton,
            PlayerOneScored,
            PlayerTwoScored,
            PlayerThreeScored,
            PlayerFourScored,
            PlayerOneExploded,
            PlayerTwoExploded,
            PlayerThreeExploded,
            PlayerFourExploded,
            PlayerOneDiedFromBees,
            PlayerTwoDiedFromBees,
            PlayerThreeDiedFromBees,
            PlayerFourDiedFromBees,
            Count
        };
    }

    static const std::array<std::string, Message::Count> messages = 
    {
        "Only One Treasure Remaining!",
        
        " is sleeping with the fishes",
        " is dining with Davy Jones",
        " got wetter than they'd like",
        " forgot they can't swim",

        " is no good at duelling",
        " met their end in a fight",
        " lost their battle",
        " passed on to the next world",

        " was haunted to death",
        " got the fright of their life",
        " got spooked",
        " was scared out of their skin.\nAnd their body.",

        " has increased their booty",
        " is coining it in",
        " has gems galore",
        " just got richer",

        " was blown away",
        " was distributed evenly",
        " sponteneously combusted",
        " experienced rapid unscheduled\ndisassembly",

        " was buzzzted",
        " was overcome by a stinging\nsensation",
        " succumbed to mis-beehaviour",
        " angered the hive"
    };
}