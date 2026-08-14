cmake_minimum_required (VERSION 3.8.2...3.20)

project(libnoson)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(MSVC)
  # This option must match the settings used in your program, in particular
  # if you are linking statically
  option(MSVC_DYNAMIC_RUNTIME "MSVC: using dynamic runtime." OFF)
  include (MSVCRuntime)
  configure_msvc_runtime()
  add_definitions ("/D_CRT_SECURE_NO_WARNINGS")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W3 /Od /RTC1")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /O2 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /Od /RTC1 /EHsc /nologo")
  set (CMAKE_SHARED_LIBRARY_PREFIX "lib")
  set (CMAKE_STATIC_LIBRARY_PREFIX "lib")
endif()

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
