/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

xygineXT - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#pragma once

#include <xyginext/core/Log.hpp>
#include <xyginext/util/String.hpp>

#include <unordered_map>
#include <string>
#include <regex>
#include <fstream>

using IniData = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

/*!
\brief Simple Windows .ini file parser
*/
class IniParse final
{
public:
    /*!
    \brief Default Constructor
    */
    IniParse() 
        : m_sectionTest("\\[(.*?)\\]")
    {

    }

    /*!
    \brief Construct and IniParse object and parse the given file if it is available
    */
    explicit IniParse(const std::string path) 
        : m_sectionTest("\\[(.*?)\\]")
    {
        parse(path);
    }

    /*!
    \brief Clears existing data and attempts to parse the given file
    */
    bool parse(const std::string path)
    {
        clear();

        std::ifstream file(path);
        if (file.is_open() && file.good())
        {
            std::string line;
            std::string current;

            while (std::getline(file, line))
            {
                trim(line);

                if (!line.empty())
                {
                    std::smatch match;
                    if (std::regex_search(line, match, m_sectionTest))
                    {
                        current = match[1];
                    }
                    else if (auto pos = line.find('='); pos != std::string::npos)
                    {
                        auto first = line.substr(0, pos);
                        trim(first);

                        auto second = line.substr(pos + 1);
                        trim(second);

                        if (!first.empty() && !second.empty())
                        {
                            m_data[current][xy::Util::String::toLower(first)] = second;
                        }
                    }
                }
            }

            file.close();
            return true;
        }

        xy::Logger::log("Failed opening: " + path, xy::Logger::Type::Error);
        return false;
    }

    std::string getValueString(const std::string& section, const std::string name)
    {
        if (m_data.count(section) != 0
            && m_data[section].count(name) != 0)
        {
            return m_data[section][name];
        }
        return {};
    }

    std::int32_t getValueInt(const std::string& section, const std::string name, std::int32_t def = 0)
    {
        if (m_data.count(section) != 0
            && m_data[section].count(name) != 0)
        {
            try
            {
                auto i = std::atoi(m_data[section][name].c_str());
                return i;
            }
            catch (...)
            {
                return def;
            }
        }
        return def;
    }

    float getValueFloat(const std::string& section, const std::string name, float def = 0.f)
    {
        if (m_data.count(section) != 0
            && m_data[section].count(name) != 0)
        {
            try
            {
                auto f = std::stof(m_data[section][name].c_str());
                return f;
            }
            catch (...)
            {
                return def;
            }
        }
        return def;
    }

    bool getValueBool(const std::string& section, const std::string name, bool def = false)
    {
        if (m_data.count(section) != 0
            && m_data[section].count(name) != 0)
        {
            return m_data[section][name] == "true";
        }
        return def;
    }

    const IniData& getData() const { return m_data; }

    void clear()
    {
        m_data.clear();
    }

private:
    IniData m_data;
    std::regex m_sectionTest;

    static inline void ltrim(std::string& s) 
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), 
            [](int ch) 
            {
                return !std::isspace(ch);
            }));
    }

    static inline void rtrim(std::string& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), 
            [](int ch) 
            {
                return !std::isspace(ch);
            }).base(), s.end());
    }

    static inline void trim(std::string& s)
    {
        ltrim(s);
        rtrim(s);
    }
};
