cmake_minimum_required (VERSION 2.8)

project(lightmapper)

# We're using c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# enable some warnings in debug builds with gcc/clang
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wreorder")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wreorder -Wheader-guard")
endif()

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(src)

add_executable(${PROJECT_NAME} ${PROJECT_SRC})
add_definitions("-D_CRT_SECURE_NO_WARNINGS -DIMGUI_IMPL_OPENGL_LOADER_GLAD")

target_link_libraries(${PROJECT_NAME} glfw ${OPENGL_LIBRARIES})
