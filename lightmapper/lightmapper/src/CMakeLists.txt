set(PROJECT_SRC 
  ${PROJECT_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/geometry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/gl_helpers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/glad.c
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scene.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_glfw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_opengl3.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
  PARENT_SCOPE)