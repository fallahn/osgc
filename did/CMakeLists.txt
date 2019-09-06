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

# Find xyginext
find_package(XYGINEXT REQUIRED)
find_package(STEAMWORKS)

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

# Create the actual binary (PROJECT_SRC variable is set inside previous steps)
add_library(${PROJECT_NAME} SHARED ${PROJECT_SRC})

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

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "osgc")
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/osgc/plugins/${PROJECT_NAME}")

set(dst_path "${CMAKE_BINARY_DIR}/osgc/plugins/${PROJECT_NAME}/")
FILE(COPY info.xgi DESTINATION ${dst_path} FILE_PERMISSIONS OWNER_READ OWNER_WRITE)
FILE(COPY assets DESTINATION ${dst_path} FILE_PERMISSIONS OWNER_READ OWNER_WRITE)