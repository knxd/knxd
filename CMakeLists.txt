# Determines what CMake APIs we can rely on
# Mainly copied from https://github.com/vcatechnology/cmake-boilerplate
cmake_minimum_required (VERSION 2.8.12)
if (${CMAKE_VERSION} VERSION_GREATER 3.2.2)
  cmake_policy(VERSION 3.2.2)
endif()
if (${CMAKE_VERSION} VERSION_GREATER 3.1 OR
    ${CMAKE_VERSION} VERSION_EQUAL 3.1)
  cmake_policy(SET CMP0054 NEW)
endif()

# Do not allow in source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Make sure we can import CMake extensions
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Load in the needed CMake modules
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
# include(AddCCompilerFlag)
include(AddCXXCompilerFlag)
# include(DetermineTargetArchitecture)
include(CMakeDependentOption)
include(GNUInstallDirs)


# Get the version
include(GetGitRevisionDescription)
git_describe(GIT_VERSION)
if(NOT GIT_VERSION)
  # No git available, use debian/changelog
  file(READ debian/changelog DEBIAN_CHANGELOG)
  string(REGEX MATCH "knxd [^-]*" DEB_VERSION "${DEBIAN_CHANGELOG}")
  string(REPLACE "knxd (" "" KNXD_VERSION "${DEB_VERSION}")
else()
  # Use the git version
  string(REGEX REPLACE "v([^-]+)-[0-9]+-g(.*)" "\\1:\\2" TOOL_VERSION "${GIT_VERSION}")
  string(REGEX REPLACE "v([^-]+)-[0-9]+-g(.*)" "\\1" KNXD_VERSION "${GIT_VERSION}")
endif()

# Set up the project
project(knxd VERSION ${KNXD_VERSION})
string(TOUPPER ${CMAKE_PROJECT_NAME} PROJECT)

# Prepare version file if version has changed
set(VERSION_DEFINE  "#define REAL_VERSION \"${TOOL_VERSION}\"")
set(VERSION_FILE ${PROJECT_BINARY_DIR}/version.h)
if(EXISTS ${VERSION_FILE})
  file(READ ${VERSION_FILE} VERSION_FILE_CONTENT)
  string(REGEX REPLACE ".*\"(.*):.*\".*" "\\1" VERSION_CURRENT ${VERSION_FILE_CONTENT})
endif()
if(NOT "${VERSION_CURRENT}" OR NOT ${KNXD_VERSION} EQUAL "${VERSION_CURRENT}")
  file(WRITE ${PROJECT_BINARY_DIR}/version.h ${VERSION_DEFINE})
endif()
message(STATUS "Project version - ${${PROJECT}_VERSION}")

# Detect the platform reliably
if(NOT MACOSX AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(DARWIN YES)
elseif(NOT BSD AND ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  SET(FREEBSD YES)
elseif(NOT LINUX AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(LINUX YES)
endif()

option(PEDANTIC_WANINGS "Enable pedantic compiler warnings" OFF)
if(PEDANTIC_WARNINGS)
  add_cxx_compiler_flag(-Wall)
  add_cxx_compiler_flag(-Wextra)
  add_cxx_compiler_flag(-Wshadow)
  add_cxx_compiler_flag(-Wsign-conversion)
  add_cxx_compiler_flag(-Wmissing-prototypes)
  add_cxx_compiler_flag(-Weverything)
  add_cxx_compiler_flag(/W4)
  add_cxx_compiler_flag(-Wno-padded)
  add_cxx_compiler_flag(/Wd4820) # padding
  add_cxx_compiler_flag(-Wno-unused-macros)
  add_cxx_compiler_flag(-Wno-format-nonliteral)
  #add_cxx_compiler_flag(-Wno-c++98-compat)
  if (MINGW)
    add_cxx_compiler_flag(-Wno-format)
  endif()
  add_cxx_compiler_flag(-Werror RELEASE)
  add_cxx_compiler_flag(/WX RELEASE)
  add_cxx_compiler_flag(-pedantic-errors)
  add_cxx_compiler_flag(-Wzero-as-null-pointer-constant)
  add_cxx_compiler_flag(-fvisibility=hidden)
  add_cxx_compiler_flag(-fstack-protector-strong RELEASE)
  add_cxx_compiler_flag(-flto RELEASE)
  add_cxx_compiler_flag(-fsanitize=undefined DEBUG)
  add_cxx_compiler_flag(-fsanitize=address DEBUG)
  if (HAVE_CXX_FLAG_FSANITIZE_ADDRESS)
    add_cxx_compiler_flag(-static-asan DEBUG)
  endif()
  add_cxx_compiler_flag(-fstack-protector-all DEBUG)
  add_cxx_compiler_flag(-mwindows)
endif()

#find_package(BISON REQUIRED)
#find_package(FLEX REQUIRED)

# Check the headers that are needed
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)

# Optional systemd support
find_package(Systemd)

find_package(argp REQUIRED)
check_include_files(linux/serial.h HAVE_LINUX_LOWLATENCY)
check_include_files(linux/rtnetlink.h HAVE_LINUX_NETLINK)
check_include_files(linux/usbdevice_fs.h HAVE_USB)
if(HAVE_USB)
  set(HAVE_LINUX_API 1)
endif()
find_package(WINSOCK)

check_symbol_exists(SA_SIZE net/route.h HAVE_SA_SIZE)
if(FREEBSD OR DARWIN)
  set(HAVE_BSD_SOURCEINFO 1)
endif()

check_function_exists(gethostbyname_r HAVE_GETHOSTBYNAME_R)
if(NOT HAVE_GETHOSTBYNAME_R)
  MESSAGE(WARNING "knxd client library not thread safe")
endif()

macro(NEED_EMI REASON)
  set(HAVE_EMI ON)
  message(STATUS "EMI because of ${REASON}")
endmacro()

option(ENABLE_BUSMONITOR "Enable bus monitoring" ON)
set(HAVE_BUSMONITOR ${ENABLE_BUSMONITOR})

option(ENABLE_MANAGEMENT "Enable management connection" OFF)
set(HAVE_MANAGEMENT ${ENABLE_MANAGEMENT})

option(ENABLE_FT12 "Enable FT1.2 backend" ON)
if(ENABLE_FT12)
  set(HAVE_FT12 ${ENABLE_FT12})
  need_emi("ft12")
endif()

option(ENABLE_DUMMY "Enable do-nothing backend" ON)
set(HAVE_DUMMY ${ENABLE_DUMMY})



option(ENABLE_TPUART "Enable TPUART driver backends" ON)
set(HAVE_TPUART ${ENABLE_TPUART})

option(ENABLE_EIBNETIP "Enable simple multicast EIBnet/IP backend" ON)
set(HAVE_EIBNETIP ${ENABLE_EIBNETIP})

option(ENABLE_EIBNETSERVER "Enable full-featured EIBnet/IP routing" ON)
set(HAVE_EIBNETSERVER ${ENABLE_EIBNETSERVER})

option(ENABLE_EIBNETIPTUNNEL "Enable EIBnet/IP tunneling backend" ON)
set(HAVE_EIBNETIPTUNNEL ${ENABLE_EIBNETIPTUNNEL})


option(ENABLE_USB "Enable USB backend" ON)
if(ENABLE_USB)
  find_package(PkgConfig)
  pkg_check_modules(LIBUSB libusb-1.0>=1.0.9 REQUIRED)
  need_emi("usb")
endif()

option(ENABLE_EIBNETIPSERVER "Enable EIBnet/IP server frontend" ON)
set(HAVE_EIBNETIPSERVER ${ENABLE_EIBNETIPSERVER})

option(ENABLE_GROUPCACHE "Enable Group Cache" ON)
set(HAVE_GROUPCACHE ${ENABLE_GROUPCACHE})

#Find the fmt package, downloading of not found TBD
find_package(fmt QUIET)
if(NOT fmt_FOUND)
  # Get the FMT from github
  message(STATUS "FMT not found, get it via git by ExternalProject_Add(fmt) TBD...")
  include(ExternalFmt)
  set(HAVE_FMT_PRINTF true)
else()
  check_include_files(fmt/printf.h HAVE_FMT_PRINTF)
endif()
set(HAVE_FMT TRUE)

# The libev
find_package(libev REQUIRED)

# Optional
find_program(indent INDENT)

# Optional Java client build
option(ENABLE_JAVA "Enable build of Java client library" OFF)
if(ENABLE_JAVA)
  set(BUILD_JAVA ${ENABLE_JAVA})
  find_package(Java REQUIRED)
endif()


include(CheckStructHasMember)
check_struct_has_member(sockaddr_in sin_len "sys/types.h;sys/socket.h;netinet/in.h" HAVE_SOCKADDR_IN_LEN)

set(RUNDIR "/var/run" CACHE PATH "Runtime directory for knxd socket")
configure_file(config.in config.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
  ${PROJECT_BINARY_DIR}/src
  )

if (CMAKE_VERSION VERSION_LESS "3.1")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
  endif ()
else ()
  set (CMAKE_CXX_STANDARD 11)
endif ()

enable_testing()
add_subdirectory(src)
add_subdirectory(systemd)
add_subdirectory(tools)
add_subdirectory(test)
