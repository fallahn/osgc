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

#include "SharedStateData.hpp"

#include <xyginext/core/FileSystem.hpp>
#include <xyginext/core/App.hpp>

#include <fstream>
#include <cstring>

namespace
{
    const std::string fileName("platform.prog");
}

void SharedData::loadProgress()
{
    reset(); //in case failed to open file

    std::string path = xy::FileSystem::getConfigDirectory(xy::App::getActiveInstance()->getApplicationName()) + fileName;
    //save file is sizeof Inventory plus the two string sizes
    
    auto expected = sizeof(Inventory) + (sizeof(std::uint64_t) * 2);
    std::ifstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        //check file is at least header size
        file.seekg(0, file.end);
        auto fileSize = file.tellg();
        file.seekg(file.beg);

        if (fileSize < expected)
        {
            xy::Logger::log(path + " - Invalid file size", xy::Logger::Type::Error);
            file.close();
            return;
        }

        //if it is try reading strings
        std::vector<char> buffer(sizeof(Inventory));
        file.read(buffer.data(), buffer.size());
        std::memcpy(&inventory, buffer.data(), buffer.size());

        buffer.resize(sizeof(std::uint64_t));
        std::uint64_t mapSize = 0;
        file.read(buffer.data(), buffer.size());
        std::memcpy(&mapSize, buffer.data(), buffer.size());

        std::uint64_t themeSize = 0;
        file.read(buffer.data(), buffer.size());
        std::memcpy(&themeSize, buffer.data(), buffer.size());

        expected += mapSize;
        expected += themeSize;

        if (fileSize != expected)
        {
            xy::Logger::log("Expected " + std::to_string(expected) + " got " + std::to_string(fileSize), xy::Logger::Type::Error);
            reset();
            file.close();
            return;
        }

        buffer.resize(mapSize);
        file.read(buffer.data(), buffer.size());
        nextMap = { buffer.data(), mapSize };

        buffer.resize(themeSize);
        file.read(buffer.data(), buffer.size());
        theme = { buffer.data(), themeSize };

        file.close();
    }
}

void SharedData::saveProgress()
{
    //save file is sizeof Inventory plus the two string sizes
    std::uint64_t mapSize = nextMap.size();
    std::uint64_t themeSize = theme.size();

    std::string path = xy::FileSystem::getConfigDirectory(xy::App::getActiveInstance()->getApplicationName()) + fileName;
    std::ofstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        std::vector<char> buffer(sizeof(Inventory) + mapSize + themeSize + (sizeof(std::uint64_t) * 2));
        auto ptr = buffer.data();

        std::memcpy(ptr, &inventory, sizeof(Inventory));
        ptr += sizeof(Inventory);

        std::memcpy(ptr, &mapSize, sizeof(mapSize));
        ptr += sizeof(mapSize);

        std::memcpy(ptr, &themeSize, sizeof(themeSize));
        ptr += sizeof(themeSize);

        std::memcpy(ptr, nextMap.data(), mapSize);
        ptr += mapSize;

        std::memcpy(ptr, theme.data(), themeSize);
        ptr += themeSize;

        file.write(buffer.data(), buffer.size());
    }
    file.close();
}

void SharedData::reset()
{
    nextMap = "gb01.tmx";
    theme = "gearboy";
    inventory = {};
}