set(PROJECT_SRC 
  ${PROJECT_SRC}
  ${CMAKE_CURRENT_SOURCE_DIR}/MainState.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MappedDevice.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/CPU6502.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Mapper.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MirroredRAM.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MMU.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/NESCart.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PluginExport.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/StateIDs.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Tia.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Util.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/VCSState.hpp
  PARENT_SCOPE)
