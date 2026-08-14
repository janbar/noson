cmake_minimum_required (VERSION 3.8.2...3.20)

project (nosontest C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

###############################################################################
# setup
if(MSVC)
  add_definitions ("/D_CRT_SECURE_NO_WARNINGS")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /RTC1")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /O2 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /RTC1 /EHsc /nologo")
endif()

set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD 11)

###############################################################################
# configure
include_directories (
  ${CMAKE_BINARY_DIR}/noson/public/noson
  ${CMAKE_BINARY_DIR}/noson/public
  ${CMAKE_SOURCE_DIR}/noson/src)

find_package(FLAC)
find_package(FLACXX)

###############################################################################
# add targets
add_executable (testdevice testdevice.cpp)
add_dependencies (testdevice noson)
target_link_libraries (testdevice noson)

add_executable (testdevicems testdevicems.cpp)
add_dependencies (testdevicems noson)
target_link_libraries (testdevicems noson)

add_executable (testsslwget testsslwget.cpp)
add_dependencies (testsslwget noson)
target_link_libraries (testsslwget noson)

if (FLACXX_FOUND AND FLAC_FOUND)
  include_directories (BEFORE SYSTEM ${FLACXX_INCLUDE_DIR})
  add_executable (tests16le2flac tests16le2flac.cpp)
  add_dependencies (tests16le2flac noson)
  target_link_libraries (tests16le2flac noson)
endif ()

###############################################################################
# add unit tests
include(UnitTestProject)

add_library(runner STATIC runner.cpp)

unittest_project(NAME test_builtins SOURCES test_builtins.cpp TARGET runner noson)
unittest_project(NAME test_compressor SOURCES test_compressor.cpp TARGET runner noson)
unittest_project(NAME test_soap_parser SOURCES test_soap_parser.cpp TARGET runner noson)
unittest_project(NAME test_intrinsic SOURCES test_intrinsic.cpp TARGET runner noson)
if (FLACXX_FOUND AND FLAC_FOUND)
  unittest_project(NAME test_flac_encoder SOURCES test_flac_encoder.cpp TARGET runner noson)
endif ()

