cmake_minimum_required (VERSION 3.8.2...3.20)

project(libnoson)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/noson)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

###############################################################################
# install targets
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
