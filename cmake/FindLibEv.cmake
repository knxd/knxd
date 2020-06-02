find_path(
  LibEv_INCLUDE_DIRS
  NAMES ev.h
  PATHS /usr/include /usr/local/include
  NO_DEFAULT_PATH)

find_library(LibEv_LIBRARIES ev)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibEv DEFAULT_MSG LibEv_LIBRARIES
                                  LibEv_INCLUDE_DIRS)
mark_as_advanced(LibEv_INCLUDE_DIRS LibEv_LIBRARIES)

if(LibEv_FOUND)
  if(NOT TARGET LibEv::LibEv)
    add_library(LibEv::LibEv UNKNOWN IMPORTED)
    set_target_properties(
      LibEv::LibEv
      PROPERTIES IMPORTED_LOCATION "${LibEv_LIBRARIES}"
                 INTERFACE_INCLUDE_DIRECTORIES "${LibEv_INCLUDE_DIRS}")
  endif()
endif()
