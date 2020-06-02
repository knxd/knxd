cmake_minimum_required(VERSION 3.11)
project(
  knxd
  VERSION 0.14.37
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(CMakePushCheckState)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(FetchContent)
include(GNUInstallDirs)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(default_build_type "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(
    STATUS
      "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE
      "${default_build_type}"
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_SOURCE_DIR}/libfmt")
check_include_file_cxx("fmt/printf.h" HAVE_FMT_PRINTF)
cmake_pop_check_state()

check_include_file(iphlpapi.h HAVE_WINDOWS_IPHELPER)
check_include_file(linux/rtnetlink.h HAVE_LINUX_NETLINK)
check_include_file(linux/usbdevice_fs.h LINUX_API)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_symbol_exists(SA_SIZE "stdint.h;sys/socket.h;net/if.h;net/route.h"
                    HAVE_SA_SIZE)
check_symbol_exists(__Apple__ "" HAVE_APPLE)
check_symbol_exists(__FreeBSD__ "" HAVE_FREEBSD)
if(HAVE_APPLE OR HAVE_FREEBSD)
  set(HAVE_BSD_SOURCEINFO 1)
endif()
check_struct_has_member(
  sockaddr_in sin_len "sys/types.h;sys/socket.h;netinet/in.h"
  HAVE_SOCKADDR_IN_LEN)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libsystemd IMPORTED_TARGET "libsystemd")
pkg_check_modules(libusb IMPORTED_TARGET "libusb-1.0")

find_package(LibEv REQUIRED)

option(BUILD_JAVA "build java client library" OFF)
option(HAVE_BUSMONITOR "enable bus monitoring" ON)
option(HAVE_EIBNETIP "enable simple multicast EIBnet/IP backend" ON)
option(HAVE_EIBNETIPSERVER "enable EIBnet/IP server frontend" ON)
option(HAVE_EIBNETIPTUNNEL "enable EIBnet/IP tunneling backend" ON)
option(HAVE_EIBNETSERVER "enable full-featured EIBnet/IP routing" ON)
option(HAVE_FT12 "enable FT1.2 backend" ON)
option(HAVE_GROUPCACHE "enable Group Cache" ON)
option(HAVE_MANAGEMENT "enable management connections" OFF)
option(HAVE_SYSTEMD "enable systemd support" "${libsystemd_FOUND}")
option(HAVE_TPUART "enable TPUART driver backends" ON)
option(HAVE_USB "enable USB backend" "${libusb_FOUND}")

add_subdirectory(src)

if(HAVE_SYSTEMD)
  add_subdirectory(systemd)
endif()

FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 6.1.1)
FetchContent_GetProperties(fmtlib)
if(NOT fmtlib_POPULATED)
  FetchContent_Populate(fmtlib)
  add_subdirectory(${fmtlib_SOURCE_DIR} ${fmtlib_BINARY_DIR})
endif()
