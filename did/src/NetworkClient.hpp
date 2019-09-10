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

//#include "Packet.hpp"
//#include <steam/steam_api.h>
//
//static inline bool pollNetwork(Packet& packet, int channel = 0)
//{
//    std::uint32_t incomingSize = 0;
//    if (SteamNetworking()->IsP2PPacketAvailable(&incomingSize))
//    {
//        //XY_ASSERT(incomingSize > 1, "packet probably too small!");
//        if (packet.bytes.size() < incomingSize)
//        {
//            packet.bytes.resize(incomingSize);
//        }
//
//        SteamNetworking()->ReadP2PPacket(&packet.bytes[0], incomingSize, &incomingSize, &packet.sender, channel);
//        packet.size = incomingSize - 1;
//
//        return true;
//    }
//    return false;
//}
//
//template <typename T>
//void sendData(std::uint8_t packetID, const T& data, CSteamID destination, EP2PSend sendType, int channel = 0)
//{
//    static std::vector<std::uint8_t> buffer(1024);
//    std::uint32_t dataSize = sizeof(T) + 1;
//    if (buffer.size() < dataSize)
//    {
//        buffer.resize(buffer.size() * 2);
//    }
//
//    buffer[0] = static_cast<std::uint8_t>(packetID);
//    std::memcpy(&buffer[1], &data, sizeof(T));
//    SteamNetworking()->SendP2PPacket(destination, buffer.data(), dataSize, sendType, channel);
//}