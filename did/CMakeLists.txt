cmake_minimum_required(VERSION 3.1)

# Rename this variable to change the project name
SET(PROJECT_NAME desert_island_duel)

# Xcode 10 will default to Mojave (must be done BEFORE project())
if (APPLE)
 set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
endif()

# Set up the project
project(${PROJECT_NAME})

message(AUTHOR_WARNING "Requires chipmunk built with FLOAT precision")

# Set the path to our find modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Some default variables which the user may change
SET(CMAKE_BUILD_TYPE        Debug CACHE STRING  "Choose the type of build (Debug or Release)")
SET(MACOS_BUNDLE            True CACHE BOOL    "True to generate a macOS bundle (experimental)")

# We're using c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# enable some warnings in debug builds with gcc/clang
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
set(CMAKE_CXX_FLAGS "-msse4.1 -msse2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wreorder -msse4.1 -msse2")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(CMAKE_CXX_FLAGS "-msse4.1 -msse2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wreorder -Wheader-guard -msse4.1 -msse2")
endif()

# Only works with SFML version 2.4 and above
SET(SFML_MIN_VERSION 2.4)
find_package(SFML ${SFML_MIN_VERSION} REQUIRED graphics window audio system)

# Find xyginext and tmxlite
find_package(XYGINEXT REQUIRED)
find_package(STEAMWORKS)
#find_package(TMXLITE REQUIRED)

# X11 is required on unices
# plus UI frameworks for message box
if (APPLE)
  find_library(CORESERVICES_LIBRARY CoreServices)
  find_library(APPKIT AppKit)
elseif(UNIX)
  find_package(X11 REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK REQUIRED gtk+-3.0)
endif()

# Additional include directories
include_directories(
  ${XYXT_INCLUDE_DIR}
  ${SFML_INCLUDE_DIR} 
  ${STEAMWORKS_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/include)

if(X11_FOUND)
  include_directories(${X11_INCLUDE_DIRS})
endif()

# Project source files
add_subdirectory(src)

if(APPLE)
  set(BOX_SRC 
    src/boxer/boxer_osx.mm)
elseif(UNIX)
  set(BOX_SRC 
    src/boxer/boxer_linux.cpp)
elseif(WIN32)
  set(BOX_SRC 
    src/boxer/boxer_win.cpp)
endif()

# Add XY_DEBUG on Debug builds
if (CMAKE_BUILD_TYPE MATCHES Debug) 
  add_definitions(-DXY_DEBUG)
endif()

# If on apple, create a nice bundle
If (APPLE AND MACOS_BUNDLE)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

set_source_files_properties( 
    ${CMAKE_SOURCE_DIR}/assets PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/macOS/icon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SRC} ${BOX_SRC} ${CMAKE_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/macOS/icon.icns)
set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE icon.icns)
else()
  add_executable(${PROJECT_NAME} ${PROJECT_SRC} ${BOX_SRC})
endif()

# Linker settings
if(X11_FOUND)
  target_link_libraries(${PROJECT_NAME}
    ${X11_LIBRARIES})
endif()

if(UNIX)
  target_link_libraries(${PROJECT_NAME}
    ${CMAKE_DL_LIBS})
endif()

target_link_libraries(${PROJECT_NAME}
  ${SFML_LIBRARIES}
  ${SFML_DEPENDENCIES}
  ${STEAMWORKS_LIBRARIES}
  ${XYXT_LIBRARIES})

if (APPLE)
  target_link_libraries(${PROJECT_NAME} ${CORESERVICES_LIBRARY} ${APPKIT})
elseif(UNIX)
  target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES})
endif()

if(UNIX AND NOT APPLE)
  target_include_directories(${PROJECT_NAME} PRIVATE ${GTK_INCLUDE_DIRS})
endif()

# Install executable - don't install the bundle to usr/lib because this requires sudo
install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION .
  BUNDLE DESTINATION ${CMAKE_SOURCE_DIR})

# Install assets folder except on Apple, where it's included in the app bundle
if (NOT (APPLE AND MACOS_BUNDLE))
  install(DIRECTORY assets
  DESTINATION .)
endif()

#Platform specific packaging
if(APPLE AND MACOS_BUNDLE)
    install(CODE " include(BundleUtilities)
		 fixup_bundle(${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.app \"\" \"/Library/Frameworks\")
		 verify_app(${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.app)")
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_CONTACT Jonny@JonnyPaton.com)
else()
    set(CPACK_GENERATOR "ZIP")
endif()

include(CPack)
