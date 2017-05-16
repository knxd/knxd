# This file is part of CMake-argp.
#
# CMake-argp is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see
#
#  http://www.gnu.org/licenses/
#
#
# Copyright (c)
#   2016-2017 Alexander Haase <ahaase@alexhaase.de>
#

include(FindPackageHandleStandardArgs)
include(CheckFunctionExists)


# Do the following checks for header, library and argp functions quietly. Only
# print the result at the end of this file.
set(CMAKE_REQUIRED_QUIET TRUE)


# First search the argp header file. If it is not found, any further steps will
# fail.
find_path(ARGP_INCLUDE_PATH "argp.h")
if (ARGP_INCLUDE_PATH)
	set(CMAKE_REQUIRED_INCLUDES "${ARGP_INCLUDE_PATH}")

	# Find the required argp library. argp may be shipped together with libc (as
	# glibc does), or as independent library (e.g. for Windows, mac OS, ...). If
	# the library was found before, the cached result will be used.
	if (NOT ARGP_LIBRARIES)
		# First check if argp is shipped together with libc. The required
		# argp_parse function should be available after linking to libc,
		# otherwise libc doesn't ship it.
		check_function_exists("argp_parse" ARGP_IN_LIBC)
		if (ARGP_IN_LIBC)
			set(ARGP_LIBRARIES "c" CACHE STRING
			    "Libraries required for using argp.")

		# argp is not shipped with libc. Try to find the argp library and check
		# if it has the required argp_parse function.
		else ()
			find_library(ARGP_LIBRARIES "argp")
			if (ARGP_LIBRARIES)
				set(CMAKE_REQUIRED_LIBRARIES "${ARGP_LIBRARIES}")
				check_function_exists("argp_parse" ARGP_EXTERNAL)
				if (NOT ARGP_EXTERNAL)
					message(FATAL_ERROR "Your system ships an argp library in "
					        "${ARGP_LIBRARIES}, but it does not have a symbol "
					        "named argp_parse.")
				endif ()
			endif ()
		endif ()
	endif ()
endif ()


# Restore the quiet settings. By default the last check should be printed if not
# disabled in the find_package call.
set(CMAKE_REQUIRED_QUIET ${argp_FIND_QUIETLY})


# Check for all required variables.
find_package_handle_standard_args(argp
	DEFAULT_MSG
	ARGP_LIBRARIES ARGP_INCLUDE_PATH)
mark_as_advanced(ARGP_LIBRARIES ARGP_INCLUDE_PATH)
