if(DEFINED included_fmt_cmake)
  return()
else()
  set(included_fmt_cmake TRUE)
endif()

set(FMT_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/fmt-build)

file(MAKE_DIRECTORY ${FMT_BUILD_DIR} ${FMT_BUILD_DIR}/build)

set(CMAKE_LIST_CONTENT "
cmake_minimum_required(VERSION 2.8)

# Enable ExternalProject CMake module
include(ExternalProject)

ExternalProject_Add(
  external_fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  CMAKE_ARGS -DFMT_TEST=OFF
  INSTALL_COMMAND \"\"
)
add_custom_target(trigger_fmt)
add_dependencies(trigger_fmt external_fmt)
")

file(WRITE ${FMT_BUILD_DIR}/CMakeLists.txt ${CMAKE_LIST_CONTENT})

execute_process(COMMAND ${CMAKE_COMMAND} ..
  WORKING_DIRECTORY ${FMT_BUILD_DIR}/build
  )
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${FMT_BUILD_DIR}/build
  )

# find_package(fmt REQUIRED CONFIG HINTS ${FMT_BUILD_DIR}/build/external_fmt-prefix/src/external_fmt-build)
# Specify include dir
set(FMT_INCLUDE_DIRS ${FMT_BUILD_DIR}/build/external_fmt-prefix/src/external_fmt/include CACHE PATH "" FORCE)

# Library
set(FMT_LIBRARY_PATH ${FMT_BUILD_DIR}/build/external_fmt-prefix/src/external_fmt-build/${CMAKE_FIND_LIBRARY_PREFIXES}fmt.a CACHE FILEPATH "" FORCE)

set(FMT_TARGET fmt::fmt)
add_library(fmt::fmt STATIC IMPORTED)
set_property(TARGET fmt::fmt PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:${FMT_INCLUDE_DIRS}>)
set_property(TARGET fmt::fmt PROPERTY IMPORTED_LOCATION ${FMT_LIBRARY_PATH})
set_property(TARGET fmt::fmt PROPERTY INTERFACE_COMPILE_OPTIONS "-std=c++11")

add_dependencies(fmt::fmt external_fmt)

# set(FMT_LIBRARIES fmt)

mark_as_advanced(
  FMT_INCLUDE_DIRS
  FMT_LIBRARIES
  FMT_TARGET

  FMT_LIBRARY_PATH
  FMT_MAIN_LIBRARY_PATH
  FMT_LIBRARY
  FMT_MAIN_LIBRARY
  )
