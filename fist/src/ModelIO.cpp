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

#include "ModelIO.hpp"

#include <xyginext/util/IO.hpp>
#include <xyginext/core/Log.hpp>

#include <SFML/Graphics/Vertex.hpp>
#include <fstream>

//binary is simply a size_t containing vertex count, followed by serialised vertices
//this relies on sf::Vertex veing a simple struct (it currently is) but I guess this might change...

bool writeModelBinary(const std::vector<sf::Vertex>& verts, const std::string& path)
{
    std::ofstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        auto vertCount = verts.size();
        file.write((char*)&vertCount, sizeof(vertCount));
        file.write((char*)verts.data(), vertCount * sizeof(sf::Vertex));
        file.close();
        return true;
    }

    return false;
}

bool readModelBinary(std::vector<sf::Vertex>& verts, const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        auto fileSize = xy::Util::IO::getFileSize(file);
        if (fileSize > sizeof(std::size_t))
        {
            std::size_t vertCount = 0;
            file.read((char*)&vertCount, sizeof(vertCount));

            if (fileSize - sizeof(vertCount) == sizeof(sf::Vertex) * vertCount)
            {
                verts.resize(vertCount);
                file.read((char*)verts.data(), sizeof(sf::Vertex) * vertCount);
                return true;
            }
            else
            {
                file.close();
                return false;
            }
        }
        else
        {
            file.close();
            return false;
        }
    }
    xy::Logger::log("Unable to open file " + path, xy::Logger::Type::Error);
    return false;
}