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

#include "Playlist.hpp"

#include <xyginext/core/FileSystem.hpp>

#include <fstream>
#include <algorithm>

using namespace Playlist;

std::vector<std::string> load(const std::string& path)
{
    std::vector<std::string> retval;

    if (!xy::FileSystem::fileExists(path))
    {
        return retval;
    }

    std::string localPath = xy::FileSystem::getFilePath(path);

    std::ifstream file(path);
    if (file.is_open() && file.good())
    {
        std::string line;

        while (std::getline(file, line))
        {
            if (!line.empty())
            {
                if (line[0] == '#')
                {
                    //this is a comment
                    continue;
                }

                //this should return false if the line is a url
                if (xy::FileSystem::fileExists(localPath + line))
                {
                    //relative file
                    retval.push_back(localPath + line);
                }
                else if (xy::FileSystem::fileExists(line))
                {
                    //absolute path
                    retval.push_back(path);
                }
            }
        }

        //remove anything with unsupported extensions
        retval.erase(std::remove_if(retval.begin(), retval.end(), 
            [](const std::string& str)
            {
                auto ext = xy::FileSystem::getFileExtension(str);
                return (ext != ".mp3") && (ext != ".ogg") && (ext != ".wav") && (ext != ".flac");
            }), retval.end());

        file.close();
    }

    return retval;
}