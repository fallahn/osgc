/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#pragma once

#if defined(_WIN32)

//windows compilers need specific (and different) keywords for export
#define OSGC_EXPORT_API __declspec(dllexport)

//for vc compilers we also need to turn off this annoying C4251 warning
#ifdef _MSC_VER
#pragma warning(disable: 4251)
#endif //_MSC_VER

#else //linux, FreeBSD, macOS

#if __GNUC__ >= 4

//gcc 4 has special keywords for showing/hiding symbols,
//the same keyword is used for both importing and exporting
#define OSGC_EXPORT_API __attribute__ ((__visibility__ ("default")))
#else

//gcc < 4 has no mechanism to explicitly hide symbols, everything's exported
#define OSGC_EXPORT_API
#endif //__GNUC__

#endif //_WIN32

namespace xy
{
    class StateStack;
}

#include <limits>
#include <cstdint>
namespace StateID
{
    //push this state to the stack to return to the front-end
    //REMEMBER TO CLEAR THE STACK FIRST!
    static constexpr std::int32_t ParentState = std::numeric_limits<std::int32_t>::max();
}

#include <any>
using SharedStateData = std::any;

#ifdef __cplusplus
extern "C" 
{

#endif

    /*!
    \brief Entry point for game plugins
    Use this function to register your game states with
    the state stack passed via the pointer.
    \param stateStack Pointer to the state stack instance with which
    to register your game states
    \param sharedData Pointer to a std::any object which can be used to share
    data between your states. This can be forwarded to the constructor of any
    of your state class when registering the state

    stateStack->registerState<MenuState>(StateID::MainMenu, *sharedData);

    The shared data must meet the requirements of std::any. The pointer is
    guaranteed by the front-end to point to a valid, empty std::any instance.

    \returns The state ID which should be pushed onto the stack when it is launched
    */
    OSGC_EXPORT_API int begin(xy::StateStack* stateStack, SharedStateData* sharedData);

    
    /*!
    \brief Called by OSGC when a plugin is unloaded.
    At the very least this needs to unregister ALL states which
    were registered with the state stack during begin
    
    stateStack->unregisterState(StateID::MainMenu);

    */
    OSGC_EXPORT_API void end(xy::StateStack* stateStack);

#ifdef __cplusplus
}
#endif