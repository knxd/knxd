if(DEFINED included_fmt_cmake)
  return()
else()
  set(included_fmt_cmake TRUE)
endif()

# Enable ExternalProject CMake module
include(ExternalProject)

ExternalProject_Add(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  CMAKE_ARGS -DFMT_TEST=OFF
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON
  LOG_CONFIGURE ON
  LOG_BUILD ON)

# Specify include dir
ExternalProject_Get_Property(fmtlib source_dir)
set(FMT_INCLUDE_DIRS ${source_dir} CACHE PATH "" FORCE)

# Library
ExternalProject_Get_Property(fmtlib binary_dir)
set(FMT_LIBRARY_PATH ${binary_dir}/fmt/${CMAKE_FIND_LIBRARY_PREFIXES}fmt.a CACHE FILEPATH "" FORCE)

set(FMT_LIBRARY fmt)
add_library(${FMT_LIBRARY} UNKNOWN IMPORTED)
set_target_properties(${FMT_LIBRARY} PROPERTIES
  IMPORTED_LOCATION ${FMT_LIBRARY_PATH}
  INTERFACE_COMPILE_OPTIONS "-std=c++11"
  )
add_dependencies(${FMT_LIBRARY} fmtlib)

set(FMT_LIBRARIES ${FMT_LIBRARY})

mark_as_advanced(
  FMT_INCLUDE_DIRS
  FMT_LIBRARIES

  FMT_LIBRARY_PATH
  FMT_MAIN_LIBRARY_PATH
  FMT_LIBRARY
  FMT_MAIN_LIBRARY
  )
