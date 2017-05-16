#.rst:
# FindSystemd
# -------
#
# Find Systemd library
#
# Try to find Systemd library on UNIX systems. The following values are defined
#
# ::
#
#   SYSTEMD_FOUND         - True if Systemd is available
#   SYSTEMD_INCLUDE_DIRS  - Include directories for Systemd
#   SYSTEMD_LIBRARIES     - List of libraries for Systemd
#   SYSTEMD_DEFINITIONS   - List of definitions for Systemd
#
#=============================================================================
# Copyright (c) 2015 Jari Vetoniemi
#
# Distributed under the OSI-approved BSD License (the "License");
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

if(NOT SYSTEMD_FOUND)
  include(FeatureSummary)
  set_package_properties(Systemd PROPERTIES
    URL "http://freedesktop.org/wiki/Software/systemd/"
    DESCRIPTION "System and Service Manager")

  find_package(PkgConfig)
  pkg_check_modules(PC_SYSTEMD QUIET libsystemd)
  find_library(SYSTEMD_LIBRARIES NAMES systemd ${PC_SYSTEMD_LIBRARY_DIRS})
  find_path(SYSTEMD_INCLUDE_DIRS systemd/sd-login.h HINTS ${PC_SYSTEMD_INCLUDE_DIRS})

  set(SYSTEMD_DEFINITIONS ${PC_SYSTEMD_CFLAGS_OTHER})

  #########################################################################################

  pkg_check_modules(EXE_SYSTEMD QUIET "systemd")
  if (EXE_SYSTEMD_FOUND AND "${SYSTEMD_SERVICES_INSTALL_DIR}" STREQUAL "")
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE}
      --variable=systemdsystemunitdir systemd
      OUTPUT_VARIABLE SYSTEMD_SERVICES_INSTALL_DIR)
    string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SERVICES_INSTALL_DIR
      "${SYSTEMD_SERVICES_INSTALL_DIR}")
  elseif (NOT SYSTEMD_FOUND AND SYSTEMD_SERVICES_INSTALL_DIR)
    message (FATAL_ERROR "Variable SYSTEMD_SERVICES_INSTALL_DIR is\
    defined, but we can't find systemd using pkg-config")
  endif()
  if (EXE_SYSTEMD_FOUND AND "${SYSTEMD_SYSUSERS_INSTALL_DIR}" STREQUAL "")
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE}
      --variable=sysusersdir systemd
      OUTPUT_VARIABLE SYSTEMD_SYSUSERS_INSTALL_DIR)
    string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SYSUSERS_INSTALL_DIR
      "${SYSTEMD_SYSUSERS_INSTALL_DIR}")
  elseif (NOT SYSTEMD_FOUND AND SYSTEMD_SYSUSERS_INSTALL_DIR)
    message (FATAL_ERROR "Variable SYSTEMD_SYSUSERS_INSTALL_DIR is\
    defined, but we can't find systemd using pkg-config")
  endif()

  if (EXE_SYSTEMD_FOUND)
    message(STATUS "systemd services install dir: ${SYSTEMD_SERVICES_INSTALL_DIR}")
  endif (EXE_SYSTEMD_FOUND)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SYSTEMD DEFAULT_MSG
    SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES SYSTEMD_SERVICES_INSTALL_DIR SYSTEMD_SYSUSERS_INSTALL_DIR)
  mark_as_advanced(SYSTEMD_INCLUDE_DIRS SYSTEMD_LIBRARIES SYSTEMD_DEFINITIONS
    SYSTEMD_SERVICES_INSTALL_DIR SYSTEMD_SYSUSERS_INSTALL_DIR)

endif()
