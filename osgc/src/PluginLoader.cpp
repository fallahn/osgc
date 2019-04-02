/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

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

#include "Game.hpp"

#include <xyginext/core/FileSystem.hpp>

#ifdef _WIN32
typedef int(__cdecl *Entry)(xy::StateStack*, SharedStateData*);
typedef void(__cdecl *Exit)(xy::StateStack*);

void Game::loadPlugin(const std::string& path)
{
    LOG("Check to see if we have a preceeding / !", xy::Logger::Type::Info);

    if (m_pluginHandle)
    {
        unloadPlugin();
    }

    auto fullPath = path + "/osgc.dll";

    m_pluginHandle = LoadLibrary(TEXT(fullPath.c_str()));

    if (m_pluginHandle)
    {
        auto entry = (Entry)GetProcAddress(m_pluginHandle, "begin");
        auto exit = (Exit)GetProcAddress(m_pluginHandle, "end");
        if (entry && exit)
        {
            //update current directory for resources
            auto currPath = m_rootPath;
            currPath += + "/" + path;
            xy::FileSystem::setCurrentDirectory(currPath);


            int reqState = entry(&m_stateStack, &m_sharedData);
            m_stateStack.clearStates();
            m_stateStack.pushState(reqState);

        }
        else
        {
            xy::Logger::log("Entry and exit point not found in " + path, xy::Logger::Type::Error);
            FreeLibrary(m_pluginHandle);
            m_pluginHandle = nullptr;
        }
    }
    else
    {
        xy::Logger::log("Unable to load plugin at " + path, xy::Logger::Type::Error);
    }
}

void Game::unloadPlugin()
{
    if (m_pluginHandle)
    {
        auto exit = (Exit)GetProcAddress(m_pluginHandle, "end");
        exit(&m_stateStack);

        m_sharedData.reset();

        //set current directory to default
        xy::FileSystem::setCurrentDirectory(m_rootPath);

        auto result = FreeLibrary(m_pluginHandle);
        if (!result)
        {
            xy::Logger::log("Unable to correctly unload current plugin!", xy::Logger::Type::Error);
        }
        else
        {
            m_pluginHandle = nullptr;
        }
    }
}

#else
#include <dlfcn.h>

void Game::loadPlugin(const std::string& path)
{
    if (m_pluginHandle)
    {
        unloadPlugin();
    }

    std::string fullPath = path;
#ifdef __APPLE__
    fullPath += "/libosgc.dylib";
#else
    fullPath += "/libosgc.so";
#endif

    m_pluginHandle = dlopen(fullPath.c_str(), RTLD_LAZY);

    if (m_pluginHandle)
    {
        int(*entryPoint)(xy::StateStack*, SharedStateData*);
        void(*exitPoint)(xy::StateStack*);

        *(int**)(&entryPoint) = (int*)dlsym(m_pluginHandle, "begin");
        *(void**)(&exitPoint) = dlsym(m_pluginHandle, "end");

        if (entryPoint && exitPoint)
        {
            //update current directory for resources
            auto currPath = m_rootPath;
            currPath += "/" + path;
            xy::FileSystem::setCurrentDirectory(currPath);

            auto reqState = entryPoint(&m_stateStack, &m_sharedData);
            m_stateStack.clearStates();
            m_stateStack.pushState(reqState);
        }
        else
        {
            xy::Logger::log("Entry and exit point not found in " + path, xy::Logger::Type::Error);
            dlclose(m_pluginHandle);
            m_pluginHandle = nullptr;
        }
    }
    else
    {
        xy::Logger::log("Unable to open plugin at " + path, xy::Logger::Type::Error);
    }
}

void Game::unloadPlugin()
{
    if (m_pluginHandle)
    {
        void(*exitPoint)(xy::StateStack*);
        *(void**)(&exitPoint) = dlsym(m_pluginHandle, "end");
        exitPoint(&m_stateStack);
        m_sharedData.reset();

        //set current directory to default
        xy::FileSystem::setCurrentDirectory(m_rootPath);

        dlclose(m_pluginHandle);
        m_pluginHandle = nullptr;
    }
}

#endif //win32