cmake_minimum_required (VERSION 2.8.12)
cmake_policy (VERSION 2.8.12)
cmake_policy(SET CMP0054 NEW)

project (nosontest C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

###############################################################################
# add definitions
if (MSVC)
  add_definitions ("/D_CRT_SECURE_NO_WARNINGS")
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /W3")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W3")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /Od /RTC1 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /Od /RTC1 /EHsc /nologo")
endif ()

set (CMAKE_CXX_STANDARD_REQUIRED ON)
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
add_executable (testdevice src/testdevice.cpp)
add_dependencies (testdevice noson)
target_link_libraries (testdevice noson)

add_executable (testdevicems src/testdevicems.cpp)
add_dependencies (testdevicems noson)
target_link_libraries (testdevicems noson)

add_executable (testsslwget src/testsslwget.cpp)
add_dependencies (testsslwget noson)
target_link_libraries (testsslwget noson)

add_executable (testintrinsic src/testintrinsic.cpp)
add_dependencies (testintrinsic noson)
target_link_libraries (testintrinsic noson)

if (FLACXX_FOUND AND FLAC_FOUND)
  include_directories (BEFORE SYSTEM ${FLACXX_INCLUDE_DIR})
  add_executable (tests16le2flac src/tests16le2flac.cpp)
  add_dependencies (tests16le2flac noson)
  target_link_libraries (tests16le2flac noson)
endif ()

###############################################################################
# add unit tests
include(UnitTestProject)

if (FLACXX_FOUND AND FLAC_FOUND)
  unittest_project(NAME check_flac_encoder SOURCES src/check_flac_encoder.cpp TARGET noson)
endif ()
unittest_project(NAME check_compressor SOURCES src/check_compressor.cpp TARGET noson)
unittest_project(NAME check_soap_parser SOURCES src/check_soap_parser.cpp TARGET noson)
unittest_project(NAME check_intrinsic SOURCES src/check_intrinsic.cpp TARGET noson)
unittest_project(NAME check_builtin SOURCES src/check_builtin.cpp TARGET noson)

