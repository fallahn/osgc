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
#include "SerialVertex.hpp"

#include <xyginext/util/IO.hpp>
#include <xyginext/core/Log.hpp>

#include <SFML/Graphics/Vertex.hpp>
#include <fstream>

//binary is simply a size_t containing vertex count, followed by serialised vertices
//this struct ensures the members are always in the correct order, even if the sf::Vertex
//format changes at some point, as well as allowing other software to know the structure
//without having to link against the sf::Vertex class
namespace
{
    SerialVertex serialise(sf::Vertex vertIn)
    {
        SerialVertex vertOut;
        vertOut.posX = vertIn.position.x;
        vertOut.posY = vertIn.position.y;
        vertOut.normX = vertIn.color.r;
        vertOut.normY = vertIn.color.g;
        vertOut.normZ = vertIn.color.b;
        vertOut.heightMultiplier = vertIn.color.a;
        vertOut.texU = vertIn.texCoords.x;
        vertOut.texV = vertIn.texCoords.y;

        return vertOut;
    }

    sf::Vertex deserialise(SerialVertex vertIn)
    {
        sf::Vertex vertOut;
        vertOut.position.x = vertIn.posX;
        vertOut.position.y = vertIn.posY;
        vertOut.color.r = vertIn.normX;
        vertOut.color.g = vertIn.normY;
        vertOut.color.b = vertIn.normZ;
        vertOut.color.a = vertIn.heightMultiplier;
        vertOut.texCoords.x = vertIn.texU;
        vertOut.texCoords.y = vertIn.texV;

        return vertOut;
    }
}

bool writeModelBinary(const std::vector<sf::Vertex>& verts, const std::string& path)
{
    std::ofstream file(path, std::ios::binary);
    if (file.is_open() && file.good())
    {
        auto vertCount = verts.size();

        std::vector<SerialVertex> buff;
        buff.reserve(vertCount);
        for (const auto& vert : verts)
        {
            buff.push_back(serialise(vert));
        }

        file.write((char*)&vertCount, sizeof(vertCount));
        file.write((char*)buff.data(), vertCount * sizeof(SerialVertex));
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

            if (fileSize - sizeof(vertCount) == sizeof(SerialVertex) * vertCount)
            {
                std::vector<SerialVertex> buff(vertCount);
                file.read((char*)buff.data(), sizeof(SerialVertex) * vertCount);

                verts.clear();
                verts.reserve(vertCount);
                for (const auto& sVert : buff)
                {
                    verts.push_back(deserialise(sVert));
                }

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